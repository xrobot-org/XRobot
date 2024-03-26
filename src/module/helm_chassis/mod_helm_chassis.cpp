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
#include <new>
#include <random>
#include <string>

#include "bsp_time.h"

#define ROTOR_WZ_MIN 0.6f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 0.8f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.0025f /* 小陀螺转动频率 */
// #define M_2PI (M_2PI)

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
HelmChassis<Motor, MotorParam>::HelmChassis(Param& param, float control_freq)
    : param_(param),
      mode_(HelmChassis::RELAX),
      follow_pid_(param.follow_pid_param, control_freq),
      ctrl_lock_(true) {
  memset(&(this->cmd_), 0, sizeof(this->cmd_));
  for (int i = 0; i < 4; i++) {
    this->change_actr_.at(i) =
        new Component::PosActuator(param.change_act_param.at(i), control_freq);
    this->change_motor_.at(i) =
        new Motor(param.change_motor_param.at(i),
                  (std::string("changeChassis_") + std::to_string(i)).c_str());
  }
  for (uint8_t i = 0; i < 4; i++) {
    this->actuator_.at(i) =
        new Component::SpeedActuator(param.actuator_param.at(i), control_freq);

    this->motor_.at(i) =
        new Motor(param.motor_param.at(i),
                  (std::string("Chassis_") + std::to_string(i)).c_str());
  }

  this->setpoint_.motor_rotational_speed =
      reinterpret_cast<float*>(System::Memory::Malloc(
          4 * sizeof(*this->setpoint_.motor_rotational_speed)));
  ASSERT(this->setpoint_.motor_rotational_speed);
  // this->setpoint_.wheel_pos = reinterpret_cast<float*>(
  //     System::Memory::Malloc(4 * sizeof(*this->setpoint_.wheel_pos)));
  // ASSERT(this->setpoint_.wheel_pos);
  auto event_callback = [](ChassisEvent event, HelmChassis* chassis) {
    chassis->ctrl_lock_.Wait(UINT32_MAX);

    switch (event) {
      case SET_MODE_RELAX:
        chassis->SetMode(RELAX);
        break;
      case SET_MODE_FOLLOW:
        chassis->SetMode(FOLLOW_GIMBAL);
        chassis->r_spin_ = 0.9f;
        break;
      case SET_MODE_ROTOR:
        chassis->SetMode(ROTOR);
        chassis->r_spin_ = 0.0f;
        break;
      case SET_MODE_INDENPENDENT:
        chassis->SetMode(INDENPENDENT);
        chassis->r_spin_ = 0.9f;
        break;
      default:
        break;
    }

    chassis->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<HelmChassis*, ChassisEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto chassis_thread = [](HelmChassis* chassis) {
    auto raw_ref_sub = Message::Subscriber<Device::Referee::Data>("referee");
    auto cmd_sub =
        Message::Subscriber<Component::CMD::ChassisCMD>("cmd_chassis");
    // auto yaw_sub = Message::Subscriber<float>("chassis_yaw");
    //   auto cap_sub = Message::Subscriber<Device::Cap::Info>("cap_info");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      raw_ref_sub.DumpData(chassis->raw_ref_);
      cmd_sub.DumpData(chassis->cmd_);
      //  yaw_sub.DumpData(chassis->yaw_);
      //   cap_sub.DumpData(chassis->cap_);
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

  this->thread_.Create(chassis_thread, this, "chassis_thread", 512,
                       System::Thread::MEDIUM);

  System::Timer::Create(this->DrawUIStatic, this, 2100);

  System::Timer::Create(this->DrawUIDynamic, this, 200);
}

template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < 4; i++) {
    this->change_motor_[i]->Update();
    this->motor_[i]->Update();
  }
}

template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = this->now_ - this->last_wakeup_;
  this->last_wakeup_ = this->now_;

  /* 计算vx、vy */
  PosBack();
  switch (this->mode_) {
    case HelmChassis::BREAK: /* 刹车模式电机停止 */
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;
    case HelmChassis::INDENPENDENT: /* 独立模式控制向量与运动向量相等
                                     */
    {
      for (uint32_t i = 0; i < 4; i++) {
        this->v[i][0] = this->cmd_.x;
        this->v[i][1] = this->cmd_.y;

        this->gyro_angle_[i] = atan2((this->v[i][0]), (this->v[i][1]));
        // if (this->gyro_angle_[i] > 0 && this->gyro_angle_[i] < M_PI) {
        //   this->real_v_[i] =
        //       -sqrt(pow(this->v[i][0], 2) + pow(this->v[i][1], 2));
        // } else {
        //   this->gyro_angle_[i] -= M_PI;
        //   this->real_v_[i] =
        //       sqrt(pow(this->v[i][0], 2) + pow(this->v[i][1], 2));
        // }
        if (this->cmd_.y == 0 && this->cmd_.x < 0) {
          this->gyro_angle_[i] += M_PI;
        }
        if (this->gyro_angle_[i] > -(M_PI / 2) &&
            this->gyro_angle_[i] <= (M_PI / 2)) {
          this->real_v_[i] =
              -sqrt(pow(this->v[i][0], 2) + pow(this->v[i][1], 2));
        } else {
          this->gyro_angle_[i] += M_PI;
          this->real_v_[i] =
              sqrt(pow(this->v[i][0], 2) + pow(this->v[i][1], 2));
        }
      }
      break;
    }
    case HelmChassis::RELAX:
      break;
    case HelmChassis::FOLLOW_GIMBAL: /* 按照云台方向换算运动向量
                                      */
    case HelmChassis::ROTOR: {
      for (uint32_t i = 0; i < 4; i++) {
        this->v[i][0] =
            this->cmd_.x * sin(this->yaw_) + this->cmd_.y * cos(this->yaw_);
        this->v[i][1] =
            this->cmd_.x * cos(this->yaw_) - this->cmd_.y * sin(this->yaw_);

        this->real_v_[i] = -sqrt(
            pow(this->v[i][0] + (0.9 - this->r_spin_) * this->r[i][0], 2) +
            pow(this->v[i][1] + (0.9 - this->r_spin_) * this->r[i][0], 2));

        this->gyro_angle_[i] =
            atan2((this->v[i][0] + (0.9 - this->r_spin_) * this->r[i][0]),
                  (this->v[i][1] + (0.9 - this->r_spin_) * this->r[i][1]));
      }
      break;
    }
    default:
      break;
  }

  /* 计算wz */
  switch (this->mode_) {
    case HelmChassis::RELAX:
    case HelmChassis::BREAK:
      break;
    case HelmChassis::INDENPENDENT: /* 独立模式wz为0 */
      // // this->move_vec_.wz = this->cmd_.z;
      // for (int i = 0; i < 4; i++) {
      //   this->setpoint_.wheel_pos[i] += this->gyro_angle_[i];
      // }
      // break;
    case HelmChassis::FOLLOW_GIMBAL: /* 跟随模式通过PID控制使车头跟随云台
                                      */
      // // this->move_vec_.wz =
      // //     this->follow_pid_.Calculate(0.0f, this->yaw_, this->dt_);

      // // if (this->move_vec_.wz < 0.01 && this->move_vec_.wz > -0.01) {
      // //   this->move_vec_.wz = 0;
      // // }

      // for (int i = 0; i < 4; i++) {
      //   this->setpoint_.wheel_pos[i] += this->gyro_angle_[i];
      // }
      // break;
    case HelmChassis::ROTOR: /* 小陀螺模式使底盘以一定速度旋转
                              */
      for (int i = 0; i < 4; i++) {
        this->setpoint_.wheel_pos[i] += this->gyro_angle_[i];
      }
      break;
    default:
      ASSERT(false);
      return;
  }

  /* 根据底盘模式计算输出值 */
  switch (this->mode_) {
    case HelmChassis::BREAK:
    case HelmChassis::FOLLOW_GIMBAL:
    case HelmChassis::ROTOR:
    case HelmChassis::INDENPENDENT: /* 独立模式,受PID控制 */ {
      this->percentage = 0.0f;
      if (cap_.online_) {
        this->percentage = cap_.percentage_;
      } else if (ref_.status == Device::Referee::RUNNING) {
        this->percentage = this->ref_.chassis_pwr_buff / 30.0f;
      } else {
        this->percentage = 1.0f;
      }
      clampf(&this->percentage, 0.0f, 1.0f);

      for (unsigned i = 0; i < 4; i++) {
        //        Component::Type::CycleValue(this->setpoint_.wheel_pos[i]);
        this->pos_out_[i] = this->change_actr_[i]->Calculate(
            this->setpoint_.wheel_pos[i], 0.0,
            this->change_motor_[i]->GetAngle(),
            this->dt_);  //原速度获取写死了为0，不知道为什么

        this->out_[i] = this->actuator_[i]->Calculate(
            this->real_v_[i] * 5000, this->motor_[i]->GetSpeed(), this->dt_);

        this->motor_[i]->Control(this->out_[i]);
        this->change_motor_[i]->Control(pos_out_[i]);
      }
      break;
    }
    case HelmChassis::RELAX: /* 放松模式,不输出 */
      for (size_t i = 0; i < 4; i++) {
        this->motor_[i]->Relax();
        this->change_motor_[i]->Relax();
      }
      break;
    default:
      ASSERT(false);
      return;
  }
}
template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::PosBack() {
  this->setpoint_.wheel_pos[0] = 5.80765152;
  this->setpoint_.wheel_pos[1] = 5.51926279;
  this->setpoint_.wheel_pos[2] = 4.88572884;
  this->setpoint_.wheel_pos[3] = 5.76700115;
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
float HelmChassis<Motor, MotorParam>::CalcWz(const float LO, const float HI) {
  float wz_vary = fabsf(0.2f * sinf(ROTOR_OMEGA * this->now_)) + LO;
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
    this->actuator_[i]->Reset();
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
