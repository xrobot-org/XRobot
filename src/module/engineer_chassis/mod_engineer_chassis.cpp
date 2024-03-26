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

#include <random>

#include "bsp_time.h"
#include "mod_chassis.hpp"

#define ROTOR_WZ_MIN 0.6f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 0.8f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.0025f /* 小陀螺转动频率 */

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
      case SET_MODE_REVERSE:
        chassis->mode_ = REVERSE;
        break;
      default:
        break;
    }

    chassis->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<Chassis*, ChassisEvent>(event_callback, this,
                                                        this->param_.EVENT_MAP);

  auto chassis_thread = [](Chassis* chassis) {
    auto cmd_sub =
        Message::Subscriber<Component::CMD::ChassisCMD>("cmd_chassis");

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      cmd_sub.DumpData(chassis->cmd_);

      /* 更新反馈值 */
      chassis->PraseRef();

      chassis->ctrl_lock_.Wait(UINT32_MAX);
      chassis->UpdateFeedback();
      chassis->Control();
      chassis->ctrl_lock_.Post();

      /* 运行结束，等待下一次唤醒 */
      chassis->thread_.SleepUntil(2);
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
  }
}

template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;
  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 计算vx、vy */
  this->yaw_ = 0.0f;
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
    case Chassis::REVERSE: {
      this->move_vec_.vx = -this->cmd_.x;
      this->move_vec_.vy = -this->cmd_.y;
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
    case Chassis::REVERSE: {
      this->move_vec_.wz = this->cmd_.z;
      break;
    }
    default:
      XB_ASSERT(false);
      return;
  }

  /* move_vec -> motor_rpm_set. 通过运动向量计算轮子转速目标值 */
  this->mixer_.Apply(this->move_vec_, this->setpoint_.motor_rotational_speed);

  /* 根据轮子转速目标值，利用PID计算电机输出值 */

  /* 根据底盘模式计算输出值 */
  switch (this->mode_) {
    case Chassis::BREAK:
    case Chassis::FOLLOW_GIMBAL:
    case Chassis::ROTOR:
    case Chassis::REVERSE:
    case Chassis::INDENPENDENT: /* 独立模式,受PID控制 */ {
      float percentage = 0.0f;
      if (cap_.online_) {
        percentage = cap_.percentage_;
      } else if (ref_.status == Device::Referee::RUNNING) {
        percentage = this->ref_.chassis_pwr_buff / 30.0f;
      } else {
        percentage = 1.0f;
      }

      clampf(&percentage, 0.0f, 1.0f);

      for (unsigned i = 0; i < this->mixer_.len_; i++) {
        float out = this->actuator_[i]->Calculate(
            this->setpoint_.motor_rotational_speed[i] *
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
      XB_ASSERT(false);
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
  float wz_vary = fabsf(0.2f * sinf(ROTOR_OMEGA * this->now_)) + LO;
  clampf(&wz_vary, LO, HI);
  return wz_vary;
}

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

template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::DrawUIStatic(
    Chassis<Motor, MotorParam>* chassis) {
  chassis->string_.Draw("CM", Component::UI::UI_GRAPHIC_OP_ADD,
                        Component::UI::UI_GRAPHIC_LAYER_CONST,
                        Component::UI::UI_GREEN, UI_DEFAULT_WIDTH * 10, 80,
                        UI_CHAR_DEFAULT_WIDTH,
                        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                              REF_UI_RIGHT_START_W),
                        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                              REF_UI_MODE_LINE1_H),
                        "CHAS  FLLW  INDT  ROTR");
  Device::Referee::AddUI(chassis->string_);

  float box_pos_left = 0.0f, box_pos_right = 0.0f;

  /* 更新底盘模式选择框 */
  switch (chassis->mode_) {
    case FOLLOW_GIMBAL:
      box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
      break;
    case ROTOR:
      box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
      break;
    case INDENPENDENT:
      box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
      break;
    case RELAX:
    case BREAK:
    default:
      box_pos_left = 0.0f;
      box_pos_right = 0.0f;
      break;
  }

  if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
    chassis->rectange_.Draw(
        "CS", Component::UI::UI_GRAPHIC_OP_ADD,
        Component::UI::UI_GRAPHIC_LAYER_CHASSIS, Component::UI::UI_GREEN,
        UI_DEFAULT_WIDTH,
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_left),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE1_H +
                              REF_UI_BOX_UP_OFFSET),
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_right),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE1_H +
                              REF_UI_BOX_BOT_OFFSET));
    Device::Referee::AddUI(chassis->rectange_);
  }
}

template <typename Motor, typename MotorParam>
void Chassis<Motor, MotorParam>::DrawUIDynamic(
    Chassis<Motor, MotorParam>* chassis) {
  float box_pos_left = 0.0f, box_pos_right = 0.0f;

  /* 更新底盘模式选择框 */
  switch (chassis->mode_) {
    case FOLLOW_GIMBAL:
      box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
      break;
    case ROTOR:
      box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
      break;
    case RELAX:

    case BREAK:

    default:
      box_pos_left = 0.0f;
      box_pos_right = 0.0f;
      break;
  }

  if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
    chassis->rectange_.Draw(
        "CS", Component::UI::UI_GRAPHIC_OP_REWRITE,
        Component::UI::UI_GRAPHIC_LAYER_CHASSIS, Component::UI::UI_GREEN,
        UI_DEFAULT_WIDTH,
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_left),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE1_H +
                              REF_UI_BOX_UP_OFFSET),
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_right),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE1_H +
                              REF_UI_BOX_BOT_OFFSET));
    Device::Referee::AddUI(chassis->rectange_);
  }
}

template class Module::Chassis<Device::RMMotor, Device::RMMotor::Param>;
