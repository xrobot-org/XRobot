#include "mod_balance.hpp"

#include "bsp_time.h"
#include "dev_referee.hpp"

#define ROTOR_WZ_MIN 0.6f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 0.8f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.003f /* 小陀螺转动频率 */

#define MOTOR_MAX_ROTATIONAL_SPEED 6000.0f /* 电机的最大转速 */

#define YAW_MAX_MOVE_SPEED (2.0f)

using namespace Module;

static constexpr uint8_t enable_pid(uint8_t ch) { return (1 << (ch)); }
static constexpr bool check_pid(uint8_t pid, uint8_t ch) {
  return ((1 << (ch)) | pid) != 0;
}

template <typename Motor, typename MotorParam>
Balance<Motor, MotorParam>::Balance(Param& param, float control_freq)
    : param_(param),
      offset_pid_(param.offset_pid, control_freq),
      speed_filter_(control_freq, param.speed_filter_cutoff_freq),
      ctrl_lock_(true) {
  constexpr auto WHELL_NAMES = magic_enum::enum_names<Wheel>();

  memset(&this->move_vec_, 0, sizeof(this->move_vec_));
  memset(&this->cmd_, 0, sizeof(this->cmd_));
  memset(&this->eulr_, 0, sizeof(this->eulr_));

  for (uint8_t i = 0; i < WHEEL_NUM; i++) {
    this->motor_.at(i) =
        new Motor(param.motor_param.at(i), WHELL_NAMES[i].data());
  }

  for (uint8_t i = 0; i < CTRL_CH_NUM; i++) {
    this->pid_[i] = new Component::PID(param.pid_param.at(i), control_freq);
  }

  auto event_callback = [](ChassisEvent event, Balance* chassis) {
    chassis->ctrl_lock_.Wait(UINT32_MAX);

    switch (event) {
      case SET_MODE_RELAX:
        chassis->SetMode(RELAX);
        break;
      case SET_MODE_FOLLOW:
        chassis->SetMode(FOLLOW_GIMBAL);
        break;
      case SET_MODE_INDENPENDENT:
        chassis->SetMode(INDENPENDENT);
        break;
      case SET_MODE_ROTOR:
        chassis->SetMode(ROTOR);
        break;
      default:
        break;
    }

    chassis->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<Balance*, ChassisEvent>(event_callback, this,
                                                        this->param_.EVENT_MAP);

  auto chassis_thread = [](Balance* chassis) {
    auto raw_ref_sub = Message::Subscriber<Device::Referee::Data>("referee");
    auto cmd_sub =
        Message::Subscriber<Component::CMD::ChassisCMD>("cmd_chassis");
    auto eulr_sub = Message::Subscriber<Component::Type::Eulr>("chassis_eulr");
    auto gyro_sub =
        Message::Subscriber<Component::Type::Vector3>("chassis_gyro");
    auto yaw_sub = Message::Subscriber<float>("chassis_yaw");
    auto leg_sub =
        Message::Subscriber<Component::Type::Polar2>("leg_whell_polor");
    auto cap_sub = Message::Subscriber<Device::Cap::Info>("cap_info");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      cmd_sub.DumpData(chassis->cmd_);
      raw_ref_sub.DumpData(chassis->raw_ref_);
      eulr_sub.DumpData(chassis->eulr_);
      gyro_sub.DumpData(chassis->gyro_);
      yaw_sub.DumpData(chassis->yaw_);
      leg_sub.DumpData(chassis->leg_);
      cap_sub.DumpData(chassis->cap_);

      /* 更新反馈值 */
      chassis->PraseRef();
      chassis->ctrl_lock_.Wait(UINT32_MAX);
      chassis->UpdateFeedback();
      chassis->UpdateStatus();
      chassis->Control();
      chassis->ctrl_lock_.Post();

      /* 运行结束，等待下一次唤醒 */
      chassis->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(chassis_thread, this, "chassis_thread",
                       MODULE_BALANCE_TASK_STACK_DEPTH, System::Thread::MEDIUM);
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::PraseRef() {
  this->ref_.chassis_power_limit =
      this->raw_ref_.robot_status.chassis_power_limit;
  this->ref_.chassis_pwr_buff = this->raw_ref_.power_heat.chassis_pwr_buff;
  this->ref_.chassis_watt = this->raw_ref_.power_heat.chassis_watt;
  this->ref_.status = this->raw_ref_.status;
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::UpdateFeedback() {
  /* 接收电机反馈 */
  for (size_t i = 0; i < WHEEL_NUM; i++) {
    this->motor_[i]->Update();
  }

  /* 更新反馈值 */
  this->feeback_[CTRL_CH_FORWARD_SPEED] = this->speed_filter_.Apply(
      (this->motor_[0]->GetSpeed() - this->motor_[1]->GetSpeed()) / 2.0f /
      MOTOR_MAX_ROTATIONAL_SPEED);
  this->feeback_[CTRL_CH_DISPLACEMENT] +=
      this->feeback_[CTRL_CH_FORWARD_SPEED] * dt_;
  this->feeback_[CTRL_CH_PITCH_ANGLE] = this->eulr_.pit;
  this->feeback_[CTRL_CH_GYRO_X] = this->gyro_.x;
  this->feeback_[CTRL_CH_YAW_ANGLE] = this->eulr_.yaw;
  this->feeback_[CTRL_CH_GYRO_Z] =
      -(this->motor_[0]->GetSpeed() + this->motor_[1]->GetSpeed()) / 2.0f /
      MOTOR_MAX_ROTATIONAL_SPEED;
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::UpdateStatus() {
  Status now_status = MOVING;

  if (cmd_.y != 0.0f) {
    now_status = MOVING;
  } else {
    now_status = STATIONARY;
  }

  if (SlipDetect()) {
    now_status = SLIP;
  }

  if (RolloverDetect()) {
    now_status = ROLLOVER;
  }

  if (LowPowerDetect()) {
    now_status = LOW_POWER;
  }

  if (now_status != status_) {
    status_ = now_status;
    feeback_[CTRL_CH_DISPLACEMENT] = 0.0f;
  }
}

template <typename Motor, typename MotorParam>
bool Balance<Motor, MotorParam>::SlipDetect() {
  if (bsp_time_get_ms() - last_detect_time_ > 0.2) {
    last_detect_time_ = bsp_time_get_ms();
    slip_counter_ = 0.0f;
    if (fabsf(gyro_.x) > 0.6f) {
      if (last_detect_dir_ * gyro_.x < 0) {
        last_detect_dir_ = -last_detect_dir_;
        slip_counter_++;
      }
    }
  }
  if (slip_counter_ >= 3) {
    return true;
  } else {
    return false;
  }
}

template <typename Motor, typename MotorParam>
bool Balance<Motor, MotorParam>::RolloverDetect() {
  // TODO:
  return false;
}

template <typename Motor, typename MotorParam>
bool Balance<Motor, MotorParam>::LowPowerDetect() {
  return (ref_.status != Device::Referee::OFFLINE) &&
         (ref_.chassis_pwr_buff < 30.0f);
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  switch (this->mode_) {
    case Balance::RELAX:
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      this->move_vec_.wz = 0.0f;
      break;
    case Balance::INDENPENDENT:
    case Balance::FOLLOW_GIMBAL:
    case Balance::ROTOR: {
      float cos_beta = cosf(yaw_);
      float sin_beta = sinf(yaw_);
      this->move_vec_.vx = cos_beta * this->cmd_.x - sin_beta * this->cmd_.y;
      this->move_vec_.vy = sin_beta * this->cmd_.x + cos_beta * this->cmd_.y;
      this->move_vec_.wz = this->cmd_.z;
      break;
    }

    default:
      break;
  }

  /* 根据底盘状态选择开启哪些pid环 */
  // TODO：enable_pid无效
  switch (status_) {
    case MOVING:
      pid_enable_ = enable_pid(CTRL_CH_FORWARD_SPEED) |
                    enable_pid(CTRL_CH_PITCH_ANGLE) |
                    enable_pid(CTRL_CH_GYRO_X) | enable_pid(CTRL_CH_YAW_ANGLE) |
                    enable_pid(CTRL_CH_GYRO_Z);
      break;
    case STATIONARY:
      pid_enable_ =
          enable_pid(CTRL_CH_DISPLACEMENT) | enable_pid(CTRL_CH_FORWARD_SPEED) |
          enable_pid(CTRL_CH_PITCH_ANGLE) | enable_pid(CTRL_CH_GYRO_X) |
          enable_pid(CTRL_CH_YAW_ANGLE) | enable_pid(CTRL_CH_GYRO_Z);
      break;
    case SLIP:
    case LOW_POWER:
    case ROLLOVER:
      pid_enable_ =
          enable_pid(CTRL_CH_PITCH_ANGLE) | enable_pid(CTRL_CH_GYRO_X);
      break;
  }

  /* 计算目标值 */
  setpoint_[CTRL_CH_DISPLACEMENT] = 0.0f;
  setpoint_[CTRL_CH_FORWARD_SPEED] = this->move_vec_.vy;
  setpoint_[CTRL_CH_PITCH_ANGLE] = param_.init_g_center;
  setpoint_[CTRL_CH_GYRO_X] = 0.0f;
  if (mode_ != FOLLOW_GIMBAL) {
  } else {
    setpoint_[CTRL_CH_YAW_ANGLE] = 0.0f;
    feeback_[CTRL_CH_YAW_ANGLE] = -yaw_;
  }
  setpoint_[CTRL_CH_GYRO_Z] = 0.0f;

  switch (mode_) {
    case RELAX:
      setpoint_[CTRL_CH_DISPLACEMENT] = 0.0f;
      setpoint_[CTRL_CH_FORWARD_SPEED] = 0.0f;
      setpoint_[CTRL_CH_PITCH_ANGLE] = 0.0f;
      setpoint_[CTRL_CH_GYRO_X] = 0.0f;
      setpoint_[CTRL_CH_YAW_ANGLE] = feeback_[CTRL_CH_YAW_ANGLE];
      setpoint_[CTRL_CH_GYRO_Z] = 0.0f;
      break;
    case FOLLOW_GIMBAL:
      setpoint_[CTRL_CH_DISPLACEMENT] = 0.0f;
      setpoint_[CTRL_CH_FORWARD_SPEED] = this->move_vec_.vy;
      setpoint_[CTRL_CH_PITCH_ANGLE] = param_.init_g_center;
      setpoint_[CTRL_CH_GYRO_X] = 0.0f;
      feeback_[CTRL_CH_YAW_ANGLE] = -yaw_;
      setpoint_[CTRL_CH_GYRO_Z] = 0.0f;
    case INDENPENDENT: {
      setpoint_[CTRL_CH_DISPLACEMENT] = 0.0f;
      setpoint_[CTRL_CH_FORWARD_SPEED] = this->move_vec_.vy;
      setpoint_[CTRL_CH_PITCH_ANGLE] = param_.init_g_center;
      setpoint_[CTRL_CH_GYRO_X] = 0.0f;
      float yaw_delta =
          (this->move_vec_.vx + this->move_vec_.wz) * YAW_MAX_MOVE_SPEED * dt_;
      setpoint_[CTRL_CH_YAW_ANGLE] =
          Component::Type::CycleValue(setpoint_[CTRL_CH_YAW_ANGLE] - yaw_delta);
      setpoint_[CTRL_CH_GYRO_Z] = 0.0f;

      break;
    }
    case ROTOR: {
      setpoint_[CTRL_CH_DISPLACEMENT] = 0.0f;
      setpoint_[CTRL_CH_FORWARD_SPEED] = this->move_vec_.vy;
      setpoint_[CTRL_CH_PITCH_ANGLE] = param_.init_g_center;
      setpoint_[CTRL_CH_GYRO_X] = 0.0f;
      setpoint_[CTRL_CH_YAW_ANGLE] = feeback_[CTRL_CH_YAW_ANGLE];
      setpoint_[CTRL_CH_GYRO_Z] = 0.03f;
      break;
    }
  }

  /* 全状态反馈控制 */
  switch (this->mode_) {
    case Balance::FOLLOW_GIMBAL:
    case Balance::INDENPENDENT:
    case Balance::ROTOR: {
      /* 位移环，微分为速度 */
      /* 使用较小K和较大D来保证静止时停在原地 */
      /* 帮助车身减速时向减速方向倾斜，速度快速减小到0 */
      /* 目标速度不为0时不应该输出 */
      if (status_ == STATIONARY) {
        output_[CTRL_CH_DISPLACEMENT] =
            -this->pid_[CTRL_CH_DISPLACEMENT]->Calculate(
                this->setpoint_[CTRL_CH_DISPLACEMENT],
                this->feeback_[CTRL_CH_DISPLACEMENT],
                this->feeback_[CTRL_CH_FORWARD_SPEED], dt_);
      } else {
        output_[CTRL_CH_DISPLACEMENT] = 0;
      }

      /* 速度环 */
      /* 帮助车身加速时向加速方向倾斜 */
      if (check_pid(pid_enable_, CTRL_CH_FORWARD_SPEED)) {
        output_[CTRL_CH_FORWARD_SPEED] =
            -this->pid_[CTRL_CH_FORWARD_SPEED]->Calculate(
                this->setpoint_[CTRL_CH_FORWARD_SPEED],
                this->feeback_[CTRL_CH_FORWARD_SPEED], dt_);
      } else {
        output_[CTRL_CH_FORWARD_SPEED] = 0;
      }

      /* pitch角度环，微分为x轴角速度 */
      if (check_pid(pid_enable_, CTRL_CH_PITCH_ANGLE)) {
        output_[CTRL_CH_PITCH_ANGLE] =
            this->pid_[CTRL_CH_PITCH_ANGLE]->Calculate(
                this->setpoint_[CTRL_CH_PITCH_ANGLE],
                this->feeback_[CTRL_CH_PITCH_ANGLE],
                this->feeback_[CTRL_CH_GYRO_X], dt_);
      } else {
        output_[CTRL_CH_PITCH_ANGLE] = 0;
      }

      /* x轴角速度环 */
      if (check_pid(pid_enable_, CTRL_CH_GYRO_X)) {
        output_[CTRL_CH_GYRO_X] = this->pid_[CTRL_CH_GYRO_X]->Calculate(
            this->setpoint_[CTRL_CH_GYRO_X], this->feeback_[CTRL_CH_GYRO_X],
            dt_);
      } else {
        output_[CTRL_CH_GYRO_X] = 0;
      }

      /* yaw角度环，微分为z轴角速度 */
      if (mode_ != ROTOR) {
        output_[CTRL_CH_YAW_ANGLE] = this->pid_[CTRL_CH_YAW_ANGLE]->Calculate(
            this->setpoint_[CTRL_CH_YAW_ANGLE],
            this->feeback_[CTRL_CH_YAW_ANGLE], this->feeback_[CTRL_CH_GYRO_Z],
            dt_);
      } else {
        output_[CTRL_CH_YAW_ANGLE] = 0;
      }

      /* z轴角速度环 */
      if (check_pid(pid_enable_, CTRL_CH_GYRO_Z)) {
        output_[CTRL_CH_GYRO_Z] = this->pid_[CTRL_CH_GYRO_Z]->Calculate(
            this->setpoint_[CTRL_CH_GYRO_Z], this->feeback_[CTRL_CH_GYRO_Z],
            dt_);
      } else {
        output_[CTRL_CH_GYRO_Z] = 0;
      }

      /* 输出加和 */
      float out_balance = 0.0f, out_yaw = 0.0f, buff_percentage = 1.0f;

      if (!cap_.online_) {
        if (ref_.status == Device::Referee::OFFLINE) {
          buff_percentage = 0.8f;
        } else if (ref_.chassis_pwr_buff > 40.0f) {
          buff_percentage = 1.0f;
        } else {
          buff_percentage = ref_.chassis_pwr_buff / 40.0f;
        }
      } else {
        if (cap_.percentage_ > 0.7f) {
          buff_percentage = 1.0f;
        } else {
          buff_percentage = cap_.percentage_ / 0.7f;
        }
      }

      for (int i = 0; i < CTRL_CH_PITCH_ANGLE; i++) {
        out_balance += output_[i] * buff_percentage;
      }

      for (int i = CTRL_CH_PITCH_ANGLE; i < CTRL_CH_YAW_ANGLE; i++) {
        out_balance += output_[i];
      }

      out_balance = offset_pid_.Calculate(0.0f, -out_balance, dt_);

      for (int i = CTRL_CH_YAW_ANGLE; i < CTRL_CH_NUM; i++) {
        out_yaw += output_[i] * buff_percentage;
      }

      motor_out_[LEFT_WHEEL] = out_balance - out_yaw;
      motor_out_[RIGHT_WHEEL] = -out_balance - out_yaw;

      clampf(&motor_out_[LEFT_WHEEL], -1.0f, 1.0f);
      clampf(&motor_out_[RIGHT_WHEEL], -1.0f, 1.0f);

      for (uint8_t i = 0; i < WHEEL_NUM; i++) {
        this->motor_[i]->Control(this->motor_out_[i]);
      }
      break;
    }

    /* 放松模式,不输出 */
    case Balance::RELAX:
      for (size_t i = 0; i < WHEEL_NUM; i++) {
        this->motor_[i]->Relax();
      }
      break;
    default:
      XB_ASSERT(false);
      return;
  }
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::SetMode(Balance::Mode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }

  /* 切换模式后重置PID和滤波器 */
  for (auto pid : pid_) {
    pid->Reset();
  }

  offset_pid_.Reset();

  speed_filter_.Reset(0.0f);

  for (auto data : setpoint_) {
    data = 0.0f;
  }

  for (auto data : feeback_) {
    data = 0.0f;
  }

  this->feeback_[CTRL_CH_DISPLACEMENT] = 0.0f;
  this->mode_ = mode;
}

template class Module::Balance<Device::RMMotor, Device::RMMotor::Param>;
template class Module::Balance<Device::RMDMotor, Device::RMDMotor::Param>;
