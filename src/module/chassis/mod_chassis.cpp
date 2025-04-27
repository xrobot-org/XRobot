/**
 * @file chassis.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 底盘模组
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "mod_chassis.hpp"

#include "bsp_time.h"

#define ROTOR_WZ_MIN 0.8f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 1.0f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.0025f /* 小陀螺转动频率 */

#define MOTOR_MAX_SPEED_COFFICIENT 1.2f /* 电机的最大转速 */
#define MOTOR_MAX_ROTATIONAL_SPEED 9600 /* 电机最大转速 */

#if POWER_LIMIT_WITH_CAP
/* 保证电容电量宏定义在正确范围内 */
#if ((CAP_PERCENT_NO_LIM < 0) || (CAP_PERCENT_NO_LIM > 100) || \
     (CAP_PERCENT_WORK < 0) || (CAP_PERCENT_WORK > 100))
#error "Cap percentage should be in the range from 0 to 100."
#endif

/* 保证电容功率宏定义在正确范围内 */
#if ((CAP_MAX_LOAD < 60) || (CAP_MAX_LOAD > 200))
#error "The capacitor power should be in in the range from 60 to 200."
#endif

static const float kCAP_PERCENTAGE_NO_LIM = (float)CAP_PERCENT_NO_LIM / 100.0f;
static const float kCAP_PERCENTAGE_WORK = (float)CAP_PERCENT_WORK / 100.0f;

#endif

using namespace Module;

template <typename Motor, typename MotorParam>
Chassis<Motor, MotorParam>::Chassis(Param& param, float control_freq)
    : param_(param),
      mode_(Chassis::RELAX),
      mixer_(param.type),
      follow_pid_(param.follow_pid_param, control_freq),
      xaccl_pid_(param.xaccl_pid_param, control_freq),
      yaccl_pid_(param.yaccl_pid_param, control_freq),
      ctrl_lock_(true) {
  memset(&(this->cmd_), 0, sizeof(this->cmd_));

  for (uint8_t i = 0; i < this->mixer_.len_; i++) {
    this->actuator_.at(i) =
        new Component::SpeedActuator(param.actuator_param.at(i), control_freq);

    this->motor_.at(i) =
        new Motor(param.motor_param.at(i),
                  (std::string("Chassis_") + std::to_string(i)).c_str());
  }

  this->setpoint_.motor_rotational_speed =
      reinterpret_cast<float*>(System::Memory::Malloc(
          this->mixer_.len_ * sizeof(*this->setpoint_.motor_rotational_speed)));
  XB_ASSERT(this->setpoint_.motor_rotational_speed);

  auto event_callback = [](ChassisEvent event, Chassis* chassis) {
    chassis->ctrl_lock_.Wait(UINT32_MAX);

    switch (event) {
      case SET_MODE_RELAX:
        chassis->SetMode(RELAX);
        break;
      case SET_MODE_FOLLOW:
        chassis->SetMode(FOLLOW_GIMBAL);
        break;
      case SET_MODE_ROTOR:
        chassis->SetMode(ROTOR);
        break;
      case SET_MODE_INDENPENDENT:
        chassis->SetMode(INDENPENDENT);
        break;
      case CHANGE_POWER_UP:
        chassis->ChangePowerlim(COMMON);
        break;
      case CHANGE_POWER_DOWN:
        chassis->ChangePowerlim(BEAST);
        break;
      default:
        break;
    }

    chassis->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<Chassis*, ChassisEvent>(event_callback, this,
                                                        this->param_.EVENT_MAP);
  auto chassis_thread = [](Chassis* chassis) {
    auto raw_ref_sub = Message::Subscriber<Device::Referee::Data>("referee");
    auto cmd_sub =
        Message::Subscriber<Component::CMD::ChassisCMD>("cmd_chassis");
    auto yaw_sub = Message::Subscriber<float>("chassis_yaw");
    auto cap_sub = Message::Subscriber<Device::Cap::Info>("cap_info");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      cmd_sub.DumpData(chassis->cmd_);
      raw_ref_sub.DumpData(chassis->raw_ref_);
      yaw_sub.DumpData(chassis->yaw_);
      cap_sub.DumpData(chassis->cap_);

      /* 更新反馈值 */
      chassis->PraseRef();

      chassis->ctrl_lock_.Wait(UINT32_MAX);
      chassis->UpdateFeedback();
      chassis->Control();
      chassis->ctrl_lock_.Post();

      /* 运行结束，等待下一次唤醒 */
      chassis->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(chassis_thread, this, "chassis_thread",
                       MODULE_CHASSIS_TASK_STACK_DEPTH, System::Thread::MEDIUM);

  System::Timer::Create(this->DrawUIStatic, this, 2100);

  System::Timer::Create(this->DrawUIDynamic, this, 200);
}

template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->motor_[i]->Update();
    this->motor_feedback_[i] = this->motor_[i]->GetSpeed();
  }
}

template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 计算vx、vy */
  switch (this->mode_) {
    case Chassis::BREAK: /* 刹车模式电机停止 */
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;

    case Chassis::INDENPENDENT: /* 独立模式控制向量与运动向量相等*/
      this->move_vec_.vx = this->cmd_.x;
      this->move_vec_.vy = this->cmd_.y;
      break;

    case Chassis::RELAX:
    case Chassis::FOLLOW_GIMBAL: /* 按照云台方向换算运动向量 */
    {
      float beta = this->yaw_;
      float cos_beta = cosf(beta);
      float sin_beta = sinf(beta);
      /*控制加速度*/
      this->move_vec_.vx = this->xaccl_pid_.Calculate(
          (cos_beta * this->cmd_.x - sin_beta * this->cmd_.y),
          this->move_vec_.vx, dt_);
      if (!cmd_.x) {
        this->xaccl_pid_.Reset();
      }
      this->move_vec_.vy = this->yaccl_pid_.Calculate(
          (sin_beta * this->cmd_.x + cos_beta * this->cmd_.y),
          this->move_vec_.vy, dt_);
      if (!cmd_.y) {
        this->yaccl_pid_.Reset();
      }
      float scalar_sum = fabs(this->move_vec_.vx) + fabs(this->move_vec_.vy);
      if (scalar_sum > 1.01f) {
        this->move_vec_.vx = this->move_vec_.vx / scalar_sum;
        this->move_vec_.vy = this->move_vec_.vy / scalar_sum;
      }
      break;
    }
    case Chassis::ROTOR: {
      float beta = this->yaw_;
      float cos_beta = cosf(beta);
      float sin_beta = sinf(beta);
      this->move_vec_.vx = cos_beta * this->cmd_.x - sin_beta * this->cmd_.y;
      this->move_vec_.vy = sin_beta * this->cmd_.x + cos_beta * this->cmd_.y;
      float scalar_sum = fabs(this->move_vec_.vx) + fabs(this->move_vec_.vy);
      if (scalar_sum > 1.01f) {
        this->move_vec_.vx = this->move_vec_.vx / scalar_sum;
        this->move_vec_.vy = this->move_vec_.vy / scalar_sum;
      }
      break;
    }
    default:
      break;
  }

  /* 计算wz */
  switch (this->mode_) {
    case Chassis::RELAX:
    case Chassis::BREAK:
    case Chassis::INDENPENDENT: /* 独立模式wz为0 */
      this->move_vec_.wz = this->cmd_.z;
      break;

    case Chassis::FOLLOW_GIMBAL: /* 跟随模式通过PID控制使车头跟随云台*/
    {
      float direction = 0.0f;
      // /*双零点*/
      // if (this->yaw_ > M_PI_2) {
      //   direction = 3.14158f;
      // }
      // if (this->yaw_ < -M_PI_2) {
      //   direction = 3.14158f;
      // }
      this->move_vec_.wz =
          this->follow_pid_.Calculate(direction, this->yaw_, this->dt_);
      clampf(&this->move_vec_.wz, -1.0f, 1.0f);
      float move_scal_sum = fabs(this->move_vec_.vx) +
                            fabs(this->move_vec_.vy) + fabs(this->move_vec_.wz);
      if (move_scal_sum > 1.01f) {
        this->move_vec_.vx =
            this->move_vec_.vx * (1 - fabs(this->move_vec_.wz));
        this->move_vec_.vy =
            this->move_vec_.vy * (1 - fabs(this->move_vec_.wz));
      }
      break;
    }
    case Chassis::ROTOR: /* 小陀螺模式使底盘根据速度大小调整旋转速度*/
    {                    /* TODO 改成实际底盘速度 */
      this->move_vec_.wz = this->wz_dir_mult_;
      float move_scal_sum = fabs(this->move_vec_.vx) +
                            fabs(this->move_vec_.vy) + fabs(this->move_vec_.wz);
      if (move_scal_sum > 1.01f) {
        this->move_vec_.wz = this->move_vec_.wz / move_scal_sum;
        this->move_vec_.vx = this->move_vec_.vx / move_scal_sum;
        this->move_vec_.vy = this->move_vec_.vy / move_scal_sum;
      }
      break;
    }
    default:
      XB_ASSERT(false);
      return;
  }

  /* move_vec -> motor_rpm_set. 通过运动向量计算轮子转速目标值 */
  this->mixer_.Apply(this->move_vec_, this->setpoint_.motor_rotational_speed);

  /* 根据底盘模式计算输出值 */
  switch (this->mode_) {
    case Chassis::BREAK:
    case Chassis::FOLLOW_GIMBAL:
    case Chassis::ROTOR:
    case Chassis::INDENPENDENT: /* 独立模式,受PID控制 */ {
      float percentage = 0.0f;
      if (ref_.status == Device::Referee::RUNNING) {
        if (ref_.chassis_pwr_buff > 30) {
          percentage = 1.0f;
        } else {
          percentage = this->ref_.chassis_pwr_buff / 30.0f;
        }
      } else {
        percentage = 1.0f;
      }

      clampf(&percentage, 0.0f, 1.0f);

      for (unsigned i = 0; i < this->mixer_.len_; i++) {
        out_.motor_out[i] = this->actuator_[i]->Calculate(
            this->setpoint_.motor_rotational_speed[i] *
                MOTOR_MAX_ROTATIONAL_SPEED,
            this->motor_[i]->GetSpeed(), this->dt_);
      }
      for (unsigned i = 0; i < this->mixer_.len_; i++) {
        if (cap_.online_) {
          LimitChassisOutPower(this->max_power_limit_, out_.motor_out,
                               motor_feedback_, this->mixer_.len_);
          this->motor_[i]->Control(out_.motor_out[i]);
        } else {
          /* 不限功率的兵种使用该底盘时
           * 注意更改chassis_power_limit至电池安全功率 */
          LimitChassisOutPower(ref_.chassis_power_limit, out_.motor_out,
                               motor_feedback_, this->mixer_.len_);
          this->motor_[i]->Control(out_.motor_out[i]);
        }
      }
      break;
    }
    case Chassis::RELAX: /* 放松模式,不输出 */
      for (size_t i = 0; i < this->mixer_.len_; i++) {
        this->motor_[i]->Relax();
      }
      break;
    default:
      XB_ASSERT(false);
      return;
  }
}

/* 功率限制 */
template <typename Motor, typename MotorParam>
bool Chassis<Motor, MotorParam>::LimitChassisOutPower(float power_limit,
                                                      float* motor_out,
                                                      float* speed_rpm,
                                                      uint32_t len) {
  if (power_limit < 0.0f) {
    return 0;
  }
  float sum_motor_power = 0.0f;
  float motor_power[4];
  for (size_t i = 0; i < len; i++) {
    motor_power[i] =
        this->param_.toque_coefficient_ * fabsf(motor_out[i]) *
            fabsf(speed_rpm[i]) +
        this->param_.speed_2_coefficient_ * speed_rpm[i] * speed_rpm[i] +
        this->param_.out_2_coefficient_ * motor_out[i] * motor_out[i];
    sum_motor_power += motor_power[i];
  }
  sum_motor_power += this->param_.constant_;
  if (sum_motor_power > power_limit) {
    for (size_t i = 0; i < len; i++) {
      motor_out[i] *= power_limit / sum_motor_power;
    }
  }
  return true;
}

/* 解析裁判系统数据 */
template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::PraseRef() {
  this->ref_.chassis_power_limit =
      this->raw_ref_.robot_status.chassis_power_limit;
  this->ref_.chassis_pwr_buff = this->raw_ref_.power_heat.chassis_pwr_buff;
  this->ref_.status = this->raw_ref_.status;
}

/* 设置底盘模式 */
template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::SetMode(Chassis::Mode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }

  if (mode == Chassis::ROTOR && this->mode_ != Chassis::ROTOR) {
    std::srand(this->now_);
    this->wz_dir_mult_ = (std::rand() % 2) ? -1 : 1;
  }
  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->actuator_[i]->Reset();
  }
  this->mode_ = mode;
}

/* 功率限制切换 */
template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::ChangePowerlim(
    Chassis::Power_Mode power_mode) {
  // TODO仔细研究正常模式和野兽模式的功率设置
  if (power_mode == this->power_mode_) {
    return;
  } /* 模式未改变直接返回 */
  if (power_mode == Chassis::COMMON) {
    this->max_power_limit_ = ref_.chassis_power_limit;
  } else {
    this->max_power_limit_ = 100.0f + 100 * 0.2 * this->cap_.percentage_;
  }
  this->power_mode_ = power_mode;
}

/* 随机转速小陀螺 结合自身超电与敌方视觉水平使用 */
template <typename Motor, typename MotorParam>
float Chassis<Motor, MotorParam>::CalcWz(const float LO, const float HI) {
  float wz_vary = fabsf(0.2f * sinf(ROTOR_OMEGA * this->now_)) + LO;
  clampf(&wz_vary, LO, HI);
  return wz_vary;
}

/*慢速刷新*/
template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::DrawUIStatic(
    Chassis<Motor, MotorParam>* chassis) {
  /* 底盘模式初始化 */
  chassis->string_.Draw("CM", Component::UI::UI_GRAPHIC_OP_ADD,
                        Component::UI::UI_GRAPHIC_LAYER_CHASSIS,
                        Component::UI::UI_CYAN, UI_DEFAULT_WIDTH * 20, 80,
                        UI_CHAR_DEFAULT_WIDTH, 1336, 749, "INIT");
  Device::Referee::AddUI(chassis->string_);
  /* 车头朝向初始化 */
  chassis->cycle_.Draw(
      "CS", Component::UI::UI_GRAPHIC_OP_ADD,
      Component::UI::UI_GRAPHIC_LAYER_CHASSIS, Component::UI::UI_CYAN,
      UI_DEFAULT_WIDTH * 7,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.5),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.5 + 260), 20);
  Device::Referee::AddUI(chassis->cycle_);
}

/* 快速刷新 */
template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::DrawUIDynamic(
    Chassis<Motor, MotorParam>* chassis) {
  if (chassis->mode_ == chassis->last_mode_) {
    /* 车头朝向ui */
    chassis->cycle_.Draw(
        "CS", Component::UI::UI_GRAPHIC_OP_REWRITE,
        Component::UI::UI_GRAPHIC_LAYER_CHASSIS, Component::UI::UI_CYAN,
        UI_DEFAULT_WIDTH * 7,
        static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.5 +
                              260 * sinf(chassis->yaw_)),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.5 +
                              260 * cosf(chassis->yaw_)),
        20);
    Device::Referee::AddUI(chassis->cycle_);
  } else {
    chassis->last_mode_ = chassis->mode_;
    char mode_ui[5] = "EROR";
    switch (chassis->mode_) {
      case RELAX:
        strncpy(mode_ui, "RELX", sizeof(mode_ui)); /*1 5!*/
        break;
      case BREAK:
        strncpy(mode_ui, "BREK", sizeof(mode_ui));
        break;
      case FOLLOW_GIMBAL:
        strncpy(mode_ui, "FOLW", sizeof(mode_ui));
        break;
      case ROTOR:
        strncpy(mode_ui, "ROTO", sizeof(mode_ui));
        break;
      case INDENPENDENT:
        strncpy(mode_ui, "INDP", sizeof(mode_ui));
        break;
      default:
        break;
    }
    chassis->string_.Draw("CM", Component::UI::UI_GRAPHIC_OP_REWRITE,
                          Component::UI::UI_GRAPHIC_LAYER_CHASSIS,
                          Component::UI::UI_CYAN, UI_DEFAULT_WIDTH * 20, 80,
                          UI_CHAR_DEFAULT_WIDTH, 1336, 749, mode_ui);
    Device::Referee::AddUI(chassis->string_);
  }
}

template class Module::Chassis<Device::RMMotor, Device::RMMotor::Param>;
