#include "mod_balance.hpp"

#include "bsp_time.h"

#define ROTOR_WZ_MIN 0.6f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 0.8f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.0015f /* 小陀螺转动频率 */

#define MOTOR_MAX_ROTATIONAL_SPEED 7000.0f /* 电机的最大转速 */

using namespace Module;

template <typename Motor, typename MotorParam>
Balance<Motor, MotorParam>::Balance(Param& param, float control_freq)
    : param_(param),
      mode_(Balance::RELAX),
      eulr_pid_(param.eulr_param, control_freq),
      gyro_pid_(param.gyro_param, control_freq),
      speed_pid_(param.speed_param, control_freq),
      follow_pid_(param.follow_pid_param, control_freq),
      comp_pid_(param.comp_pid_param, control_freq),
      center_filter_(control_freq, param.center_filter_cutoff_freq),
      ctrl_lock_(true) {
  memset(&(this->cmd_), 0, sizeof(this->cmd_));

  this->setpoint_.angle.g_center = param.init_g_center;

  constexpr auto WHELL_NAMES = magic_enum::enum_names<Wheel>();

  for (uint8_t i = 0; i < WHEEL_NUM; i++) {
    this->wheel_actr_[i] = static_cast<Component::SpeedActuator*>(
        System::Memory::Malloc(sizeof(Component::SpeedActuator)));
    new (this->wheel_actr_[i])
        Component::SpeedActuator(param.wheel_param[i], control_freq);

    this->motor_[i] =
        static_cast<Motor*>(System::Memory::Malloc(sizeof(Motor)));
    new (this->motor_[i])
        Motor(param.motor_param[i],
              (std::string("Chassis_Wheel_") + WHELL_NAMES[i].data()).c_str());
  }

  auto event_callback = [](uint32_t event, void* arg) {
    Balance* chassis = static_cast<Balance*>(arg);

    chassis->ctrl_lock_.Take(UINT32_MAX);

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
      default:
        break;
    }

    chassis->ctrl_lock_.Give();
  };

  Component::CMD::RegisterEvent(event_callback, this, this->param_.EVENT_MAP);

  auto chassis_thread = [](Balance* chassis) {
    auto cmd_sub = Message::Subscriber("cmd_chassis", chassis->cmd_);
    auto eulr_sub = Message::Subscriber("chassis_eulr", chassis->eulr_);
    auto gyro_sub = Message::Subscriber("chassis_gyro", chassis->gyro_);

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      cmd_sub.DumpData();
      eulr_sub.DumpData();
      gyro_sub.DumpData();

      /* 更新反馈值 */
      chassis->ctrl_lock_.Take(UINT32_MAX);
      chassis->UpdateFeedback();
      chassis->Control();
      chassis->ctrl_lock_.Give();

      /* 运行结束，等待下一次唤醒 */
      chassis->thread_.SleepUntil(2);
    }
  };

  this->thread_.Create(chassis_thread, this, "chassis_thread", 384,
                       System::Thread::MEDIUM);
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < WHEEL_NUM; i++) {
    this->motor_[i]->Update();
  }

  this->feeback_.forward_speed = (this->motor_[RIGHT_WHEEL]->GetSpeed() -
                                  this->motor_[LEFT_WHEEL]->GetSpeed()) /
                                 2.0f / MOTOR_MAX_ROTATIONAL_SPEED;
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = this->now_ - this->last_wakeup_;
  this->last_wakeup_ = this->now_;

  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 计算vx、vy */
  switch (this->mode_) {
    case Balance::RELAX:
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;
    case Balance::ROTOR:
    case Balance::FOLLOW_GIMBAL:
      this->move_vec_.vx = this->cmd_.x;
      this->move_vec_.vy = this->cmd_.y;
      break;

    default:
      break;
  }

  /* 计算wz */
  switch (this->mode_) {
    case Balance::RELAX:
    case Balance::FOLLOW_GIMBAL:
    case Balance::ROTOR:
      this->move_vec_.wz = 0;
      break;

    default:
      ASSERT(false);
      return;
  }

  /* 根据底盘模式计算输出值 */
  switch (this->mode_) {
    case Balance::FOLLOW_GIMBAL:
    case Balance::ROTOR: {
      /* 速度环 */
      this->setpoint_.wheel_speed.speed = -this->speed_pid_.Calculate(
          this->move_vec_.vy, this->feeback_.forward_speed, this->dt_);

      /* 加减速时保证稳定姿态 */
      this->setpoint_.angle.g_comp = this->comp_pid_.Calculate(
          this->move_vec_.vy, this->feeback_.forward_speed, this->dt_);

      /* 速度误差积分估计重心 */
      this->setpoint_.angle.g_center += this->center_filter_.Apply(
          (this->feeback_.forward_speed - this->move_vec_.vy) * this->dt_);

      clampf(&(this->setpoint_.angle.g_center), -0.05 + param_.init_g_center,
             0.05 + param_.init_g_center);

      /* 直立环 */
      this->setpoint_.wheel_speed.balance = this->eulr_pid_.Calculate(
          this->setpoint_.angle.g_center - this->setpoint_.angle.g_comp,
          this->eulr_.pit, this->gyro_.x, this->dt_);

      this->setpoint_.wheel_speed.balance +=
          this->gyro_pid_.Calculate(0.0f, this->gyro_.x, this->dt_);

      /* 角度环 */
      this->setpoint_.wheel_speed.angle[LEFT_WHEEL] = -this->move_vec_.vx;
      this->setpoint_.wheel_speed.angle[RIGHT_WHEEL] = this->move_vec_.vx;

      /* 轮子速度控制 */
      for (uint8_t i = 0; i < WHEEL_NUM; i++) {
        float speed_sp = this->setpoint_.wheel_speed.speed +
                         this->setpoint_.wheel_speed.balance +
                         this->setpoint_.wheel_speed.angle[i];

        clampf(&speed_sp, -1.0f, 1.0f);

        if (i == LEFT_WHEEL) {
          speed_sp = -speed_sp;
        }

        this->motor_out[i] = this->wheel_actr_[i]->Calculate(
            speed_sp * MOTOR_MAX_ROTATIONAL_SPEED, this->motor_[i]->GetSpeed(),
            this->dt_);
      }

      /* 电机输出 */
      for (uint8_t i = 0; i < WHEEL_NUM; i++) {
        this->motor_[i]->Control(this->motor_out[i]);
      }
      break;
    }

    case Balance::RELAX: /* 放松模式,不输出 */
      for (size_t i = 0; i < WHEEL_NUM; i++) {
        this->motor_[i]->Relax();
      }
      break;
    default:
      ASSERT(false);
      return;
  }
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::SetMode(Balance::Mode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < WHEEL_NUM; i++) {
    this->wheel_actr_[i]->Reset();
  }

  this->eulr_pid_.Reset();
  this->gyro_pid_.Reset();
  this->speed_pid_.Reset();
  this->mode_ = mode;

  this->setpoint_.angle.g_center = param_.init_g_center;
}

template class Module::Balance<Device::RMMotor, Device::RMMotor::Param>;
