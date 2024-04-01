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

#include "mod_helm_chassis.hpp"

#include <math.h>
#include <sys/_stdint.h>

#include <cmath>
#include <comp_actuator.hpp>
#include <comp_type.hpp>
#include <comp_utils.hpp>
#include <cstdint>
#include <cstdlib>
#include <memory.hpp>
#include <string>

#include "bsp_time.h"

#define ROTOR_WZ_MIN 0.6f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 0.8f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.0025f /* 小陀螺转动频率 */

#define WZ_MAX_OMEGA 6.0f /* 小陀螺转动频率 */

// #define M_2PI (M_2PI)

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

#define MOTOR_MAX_ROTATIONAL_SPEED 7000.0f /* 电机的最大转速 */

using namespace Module;

template <typename Motor, typename MotorParam>
HelmChassis<Motor, MotorParam>::HelmChassis(Param& param, float control_freq)
    : param_(param), mode_(HelmChassis::RELAX), ctrl_lock_(true) {
  memset(&(this->cmd_), 0, sizeof(this->cmd_));
  for (int i = 0; i < 4; i++) {
    this->pos_actr_.at(i) =
        new Component::PosActuator(param.pos_param.at(i), control_freq);
    this->pos_motor_.at(i) =
        new Motor(param.pos_motor_param.at(i),
                  (std::string("Chassis_pos_") + std::to_string(i)).c_str());
  }

  for (uint8_t i = 0; i < 4; i++) {
    this->speed_actr_.at(i) =
        new Component::SpeedActuator(param.speed_param.at(i), control_freq);

    this->speed_motor_.at(i) =
        new Motor(param.speed_motor_param.at(i),
                  (std::string("Chassis_speed_") + std::to_string(i)).c_str());
  }
  ctrl_lock_.Post();

  auto event_callback = [](ChassisEvent event, HelmChassis* chassis) {
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
      default:
        break;
    }

    chassis->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<HelmChassis*, ChassisEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto chassis_thread = [](HelmChassis* chassis) {
    auto cmd_sub =
        Message::Subscriber<Component::CMD::ChassisCMD>("cmd_chassis");
    auto yaw_sub = Message::Subscriber<float>("chassis_yaw");
    auto raw_ref_sub = Message::Subscriber<Device::Referee::Data>("referee");
    auto cap_sub = Message::Subscriber<Device::Cap::Info>("cap_info");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      cmd_sub.DumpData(chassis->cmd_);
      yaw_sub.DumpData(chassis->yaw_);
      raw_ref_sub.DumpData(chassis->raw_ref_);
      cap_sub.DumpData(chassis->cap_);

      chassis->ctrl_lock_.Wait(UINT32_MAX);
      /* 更新反馈值 */
      chassis->PraseRef();
      chassis->UpdateFeedback();
      chassis->Control();
      chassis->ctrl_lock_.Post();

      /* 运行结束，等待下一次唤醒 */
      chassis->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(chassis_thread, this, "chassis_thread", 1024,
                       System::Thread::MEDIUM);

  System::Timer::Create(this->DrawUIStatic, this, 2100);

  System::Timer::Create(this->DrawUIDynamic, this, 200);
}

template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::PraseRef() {
  this->ref_.chassis_power_limit =
      this->raw_ref_.robot_status.chassis_power_limit;
  this->ref_.chassis_pwr_buff = this->raw_ref_.power_heat.chassis_pwr_buff;
  this->ref_.chassis_watt = this->raw_ref_.power_heat.chassis_watt;
  this->ref_.status = this->raw_ref_.status;
}

template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < 4; i++) {
    this->pos_motor_[i]->Update();
    this->speed_motor_[i]->Update();
  }
}

template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  /* 计算vx、vy */
  switch (this->mode_) {
    case HelmChassis::BREAK: /* 刹车/放松模式电机停止 */
    case HelmChassis::RELAX:
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;
      /* 独立模式控制向量与运动向量相等 */
    case HelmChassis::INDENPENDENT:
    case HelmChassis::FOLLOW_GIMBAL: {
      float tmp = sqrtf(cmd_.x * cmd_.x + cmd_.y * cmd_.y) * 1.41421f / 2.0f;

      clampf(&tmp, -1.0f, 1.0f);

      this->move_vec_.vx = 0;
      this->move_vec_.vy = tmp;
      if (tmp >= 0.1) {
        this->direct_offset_ = -(atan2(cmd_.y, cmd_.x) - M_PI / 2.0f);
      } else {
        this->direct_offset_ = 0;
      }
      break;
    }
    case HelmChassis::ROTOR: {
      float beta = this->yaw_;
      float cos_beta = cosf(beta);
      float sin_beta = sinf(beta);
      this->move_vec_.vx = cos_beta * this->cmd_.x - sin_beta * this->cmd_.y;
      this->move_vec_.vy = sin_beta * this->cmd_.x + cos_beta * this->cmd_.y;
      break;
    }
  }

  /* 计算wz */
  switch (this->mode_) {
    case HelmChassis::RELAX:
    case HelmChassis::BREAK:
      this->move_vec_.wz = 0;
      break;
    case HelmChassis::INDENPENDENT:
      /* 独立模式每个轮子的方向相同，wz当作轮子转向角速度 */
      this->move_vec_.wz = this->cmd_.z;
      this->main_direct_ -= this->move_vec_.wz * WZ_MAX_OMEGA * dt_;
      break;
    case HelmChassis::FOLLOW_GIMBAL:
      /* 跟随模式每个轮子的方向与云台相同 */
      this->move_vec_.wz = 0;
      this->main_direct_ = -yaw_;
      break;
    case HelmChassis::ROTOR:
      /* 陀螺模式底盘以一定速度旋转 */
      this->move_vec_.wz =
          this->wz_dir_mult_ * CalcWz(ROTOR_WZ_MIN, ROTOR_WZ_MAX);
      break;
    default:
      XB_ASSERT(false);
      return;
  }

  /* 根据底盘模式计算电机目标角度和速度 */
  switch (this->mode_) {
    case HelmChassis::RELAX:
      for (int i = 0; i < 4; i++) {
        pos_motor_[i]->Relax();
        speed_motor_[i]->Relax();
      }
      return;
    case HelmChassis::BREAK:
      for (auto& speed : setpoint_.motor_rotational_speed) {
        speed = 0.0f;
      }
      break;
    case HelmChassis::FOLLOW_GIMBAL:
    case HelmChassis::INDENPENDENT: /* 独立模式,受PID控制 */
      for (auto& angle : setpoint_.wheel_pos) {
        angle = main_direct_ + direct_offset_;
      }
      for (auto& speed : setpoint_.motor_rotational_speed) {
        speed = MOTOR_MAX_ROTATIONAL_SPEED * move_vec_.vy;
      }
      break;
    case HelmChassis::ROTOR: {
      float x = 0, y = 0, wheel_pos = 0;
      for (int i = 0; i < 4; i++) {
        wheel_pos = -i * M_PI / 2.0f + M_PI / 4.0f;
        x = sinf(wheel_pos) * move_vec_.wz + move_vec_.vx;
        y = cosf(wheel_pos) * move_vec_.wz + move_vec_.vy;
        setpoint_.wheel_pos[i] = -(atan2(y, x) - M_PI / 2.0f);
        setpoint_.motor_rotational_speed[i] =
            MOTOR_MAX_ROTATIONAL_SPEED * sqrtf(x * x + y * y) * 1.41421f / 2.0f;
      }
      break;
    }
  }

  /* 寻找电机最近角度 */
  for (int i = 0; i < 4; i++) {
    if (fabs(Component::Type::CycleValue(pos_motor_[i]->GetAngle() -
                                         param_.mech_zero[i]) -
             setpoint_.wheel_pos[i]) > M_PI / 2.0f) {
      motor_reverse_[i] = true;
    } else {
      motor_reverse_[i] = false;
    }
  }

  /* 输出计算 */
  for (int i = 0; i < 4; i++) {
    if (motor_reverse_[i]) {
      speed_motor_out_[i] =
          speed_actr_[i]->Calculate(-setpoint_.motor_rotational_speed[i],
                                    speed_motor_[i]->GetSpeed(), dt_);
      pos_motor_out_[i] = pos_actr_[i]->Calculate(
          setpoint_.wheel_pos[i] + M_PI + this->param_.mech_zero[i],
          pos_motor_[i]->GetSpeed(), pos_motor_[i]->GetAngle(), dt_);
    } else {
      speed_motor_out_[i] =
          speed_actr_[i]->Calculate(setpoint_.motor_rotational_speed[i],
                                    speed_motor_[i]->GetSpeed(), dt_);
      pos_motor_out_[i] = pos_actr_[i]->Calculate(
          setpoint_.wheel_pos[i] + this->param_.mech_zero[i],
          pos_motor_[i]->GetSpeed(), pos_motor_[i]->GetAngle(), dt_);
    }
  }

  float percentage = 0.0f;
  if (cap_.online_) {
    percentage = cap_.percentage_;
  } else if (ref_.status == Device::Referee::RUNNING) {
    percentage = this->ref_.chassis_pwr_buff / 60.0f;
  } else {
    percentage = 1.0f;
  }

  clampf(&percentage, 0.0f, 1.0f);
  /* 控制 */
  for (int i = 0; i < 4; i++) {
    speed_motor_[i]->Control(speed_motor_out_[i] * percentage);
  }

  clampf(&percentage, 0.5f, 1.0f);

  for (int i = 0; i < 4; i++) {
    pos_motor_[i]->Control(pos_motor_out_[i] * percentage);
  }
}

template <typename Motor, typename MotorParam>
float HelmChassis<Motor, MotorParam>::CalcWz(const float LO, const float HI) {
  float wz_vary = fabsf(0.2f * sinf(ROTOR_OMEGA *
                                    (static_cast<float>(bsp_time_get_ms())))) +
                  LO;
  clampf(&wz_vary, LO, HI);
  return wz_vary;
}

template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::SetMode(HelmChassis::Mode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }

  if (mode == HelmChassis::ROTOR && this->mode_ != HelmChassis::ROTOR) {
    std::srand(this->now_);
    this->wz_dir_mult_ = (std::rand() % 2) ? -1 : 1;
  }

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < 4; i++) {
    this->speed_actr_[i]->Reset();
    this->pos_actr_[i]->Reset();
  }

  this->mode_ = mode;
}

template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::DrawUIStatic(
    HelmChassis<Motor, MotorParam>* chassis) {
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
void HelmChassis<Motor, MotorParam>::DrawUIDynamic(
    HelmChassis<Motor, MotorParam>* chassis) {
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

template class Module::HelmChassis<Device::RMMotor, Device::RMMotor::Param>;
