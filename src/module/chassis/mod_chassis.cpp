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

#define ROTOR_WZ_MIN 0.6f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 0.8f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.0015f /* 小陀螺转动频率 */

#define MOTOR_MAX_ROTATIONAL_SPEED 7000.0f /* 电机的最大转速 */

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
      ctrl_lock_(true) {
  memset(&(this->cmd_), 0, sizeof(this->cmd_));

  for (uint8_t i = 0; i < this->mixer_.len_; i++) {
    this->actuator_[i] = static_cast<Component::SpeedActuator*>(
        System::Memory::Malloc(sizeof(Component::SpeedActuator)));
    new (this->actuator_[i])
        Component::SpeedActuator(param.actuator_param[i], control_freq);

    this->motor_[i] = (Motor*)System::Memory::Malloc(sizeof(Motor));
    new (this->motor_[i])
        Motor(param.motor_param[i],
              (std::string("Chassis_") + std::to_string(i)).c_str());
  }

  this->setpoint.motor_rotational_speed = (float*)System::Memory::Malloc(
      this->mixer_.len_ * sizeof(*this->setpoint.motor_rotational_speed));
  ASSERT(this->setpoint.motor_rotational_speed);

  auto event_callback = [](uint32_t event, void* arg) {
    Chassis* chassis = static_cast<Chassis*>(arg);

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
      case SET_MODE_INDENPENDENT:
        chassis->SetMode(INDENPENDENT);
        break;
      default:
        break;
    }

    chassis->ctrl_lock_.Give();
  };

  Component::CMD::RegisterEvent(event_callback, this, this->param_.EVENT_MAP);

  auto chassis_thread = [](Chassis* chassis) {
    auto raw_ref_sub = Message::Subscriber("referee", chassis->raw_ref_);

    auto yaw_sub = Message::Subscriber("chassis_yaw", chassis->yaw_);

    auto cmd_sub = Message::Subscriber("cmd_chassis", chassis->cmd_);

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      yaw_sub.DumpData();
      raw_ref_sub.DumpData();
      cmd_sub.DumpData();

      /* 更新反馈值 */
      chassis->PraseRef();

      chassis->ctrl_lock_.Take(UINT32_MAX);
      chassis->UpdateFeedback();
      chassis->Control();
      chassis->ctrl_lock_.Give();

      /* 运行结束，等待下一次唤醒 */
      chassis->thread_.SleepUntil(2);
    }
  };

  this->thread_.Create(chassis_thread, this, "chassis_thread", 384,
                       System::Thread::Medium);
}

template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->motor_[i]->Update();
  }
}

template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = this->now_ - this->last_wakeup_;
  this->last_wakeup_ = this->now_;

  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 计算vx、vy */
  switch (this->mode_) {
    case Chassis::BREAK: /* 刹车模式电机停止 */
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;

    case Chassis::INDENPENDENT: /* 独立模式控制向量与运动向量相等
                                 */
      this->move_vec_.vx = this->cmd_.x;
      this->move_vec_.vy = this->cmd_.y;
      break;

    case Chassis::RELAX:
    case Chassis::FOLLOW_GIMBAL: /* 按照云台方向换算运动向量
                                  */
    case Chassis::ROTOR: {
      float beta = this->yaw_;
      float cos_beta = cosf(beta);
      float sin_beta = sinf(beta);
      this->move_vec_.vx = cos_beta * this->cmd_.x - sin_beta * this->cmd_.y;
      this->move_vec_.vy = sin_beta * this->cmd_.x + cos_beta * this->cmd_.y;
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

    case Chassis::FOLLOW_GIMBAL: /* 跟随模式通过PID控制使车头跟随云台
                                  */
      this->move_vec_.wz =
          this->follow_pid_.Calculate(0.0f, this->yaw_, this->dt_);
      break;

    case Chassis::ROTOR: { /* 小陀螺模式使底盘以一定速度旋转
                            */
      this->move_vec_.wz =
          this->wz_dir_mult_ * CalcWz(ROTOR_WZ_MIN, ROTOR_WZ_MAX);
      break;
    }
    default:
      ASSERT(false);
      return;
  }

  /* move_vec -> motor_rpm_set. 通过运动向量计算轮子转速目标值 */
  this->mixer_.Apply(this->move_vec_, this->setpoint.motor_rotational_speed);

  /* 根据轮子转速目标值，利用PID计算电机输出值 */

  /* 根据底盘模式计算输出值 */
  switch (this->mode_) {
    case Chassis::BREAK:
    case Chassis::FOLLOW_GIMBAL:
    case Chassis::ROTOR:
    case Chassis::INDENPENDENT: /* 独立模式,受PID控制 */ {
      float buff_percentage = this->ref_.chassis_pwr_buff / 30.0f;
      clampf(&buff_percentage, 0.0f, 1.0f);
      for (unsigned i = 0; i < this->mixer_.len_; i++) {
        float out = this->actuator_[i]->Calculate(
            this->setpoint.motor_rotational_speed[i] *
                MOTOR_MAX_ROTATIONAL_SPEED,
            this->motor_[i]->GetSpeed(), this->dt_);

        this->motor_[i]->Control(out);
      }

      break;
    }
    case Chassis::RELAX: /* 放松模式,不输出 */
      for (size_t i = 0; i < this->mixer_.len_; i++) {
        this->motor_[i]->Relax();
      }
      break;
    default:
      ASSERT(false);
      return;
  }
}

template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::PraseRef() {
  this->ref_.chassis_power_limit =
      this->raw_ref_.robot_status.chassis_power_limit;
  this->ref_.chassis_pwr_buff = this->raw_ref_.power_heat.chassis_pwr_buff;
  this->ref_.chassis_watt = this->raw_ref_.power_heat.chassis_watt;
  this->ref_.status = this->raw_ref_.status;
}

template <typename Motor, typename MotorParam>
float Chassis<Motor, MotorParam>::CalcWz(const float LO, const float HI) {
  float wz_vary = fabsf(0.2f * sinf(ROTOR_OMEGA * (float)this->now_)) + LO;
  clampf(&wz_vary, LO, HI);
  return wz_vary;
}

template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::SetMode(Chassis::Mode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }

  if (mode == Chassis::ROTOR && this->mode_ != Chassis::ROTOR) {
    srand(this->now_);
    this->wz_dir_mult_ = (rand() % 2) ? -1 : 1;
  }
  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->actuator_[i]->Reset();
  }
  this->mode_ = mode;
}

template class Module::Chassis<Device::RMMotor, Device::RMMotor::Param>;
