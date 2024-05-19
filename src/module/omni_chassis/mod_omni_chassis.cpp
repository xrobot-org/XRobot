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

#include "mod_omni_chassis.hpp"

#include <math.h>

#include <random>

#include "bsp_time.h"

#define ROTOR_WZ_MIN 0.8f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 1.0f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.0025f /* 小陀螺转动频率 */

#define MOTOR_MAX_SPEED_COFFICIENT 1.2f /* 电机的最大转速 */

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
OmniChassis<Motor, MotorParam>::OmniChassis(Param& param, float control_freq)
    : param_(param),
      mode_(OmniChassis::RELAX),
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

  auto event_callback = [](ChassisEvent event, OmniChassis* chassis) {
    chassis->ctrl_lock_.Wait(UINT32_MAX);

    switch (event) {
      case SET_MODE_RELAX:
        chassis->SetMode(RELAX);
        break;
      case SET_MODE_INTERSECT:
        chassis->SetMode(FOLLOW_GIMBAL_INTERSECT);
        chassis->param_.type = Component::Mixer::OMNIPLUS;

        break;
      case SET_MODE_CROSS:
        chassis->SetMode(FOLLOW_GIMBAL_CROSS);
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

  Component::CMD::RegisterEvent<OmniChassis*, ChassisEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto chassis_thread = [](OmniChassis* chassis) {
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

  this->thread_.Create(chassis_thread, this, "chassis_thread", 512,
                       System::Thread::MEDIUM);

  System::Timer::Create(this->DrawUIStatic, this, 2100);

  System::Timer::Create(this->DrawUIDynamic, this, 200);
}

template <typename Motor, typename MotorParam>
void OmniChassis<Motor, MotorParam>::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->motor_[i]->Update();
    this->motor_feedback_.motor_speed[i] = this->motor_[i]->GetSpeed();
  }
}
template <typename Motor, typename MotorParam>
uint16_t OmniChassis<Motor, MotorParam>::MAXSPEEDGET(float power_limit) {
  uint16_t speed = 0.0f;
  if (power_limit <= 50.0f) {
    speed = 5000;
  } else if (power_limit <= 60.0f) {
    speed = 5500;
  } else if (power_limit <= 70.0f) {
    speed = 5500;
  } else if (power_limit <= 80.0f) {
    speed = 6500;
  } else if (power_limit <= 100.0f) {
    speed = 7000;
  } else {
    speed = 7500;
  }
  return speed;
}
template <typename Motor, typename MotorParam>
bool OmniChassis<Motor, MotorParam>::LimitChassisOutput(float power_limit,
                                                        float* motor_out,
                                                        float* speed_rpm,
                                                        uint32_t len) {
  if (power_limit < 0.0f) {
    return 0;
  }

  float motor_3508_power[4];
  float sum_motor_out = 0.0f;
  float power_scal = 0.0f;
  for (size_t i = 0; i < len; i++) {
    motor_3508_power[i] =
        this->param_.toque_coefficient_ * fabsf(motor_out[i]) *
            fabsf(speed_rpm[i]) +
        this->param_.speed_2_coefficient_ * speed_rpm[i] * speed_rpm[i] +
        this->param_.out_2_coefficient_ * motor_out[i] * motor_out[i] +
        this->param_.constant_;
    sum_motor_out += motor_3508_power[i];
  }
  if (sum_motor_out >= power_limit) {
    power_scal = power_limit / sum_motor_out;
    for (size_t i = 0; i < len; i++) {
      motor_3508_power[i] *= power_scal;
      float b = this->param_.toque_coefficient_ * fabsf(speed_rpm[i]);
      float c =
          this->param_.speed_2_coefficient_ * speed_rpm[i] * speed_rpm[i] -
          motor_3508_power[i] + this->param_.constant_;
      if (motor_out[i] >= 0) {
        float out =
            (-b + sqrtf(b * b - 4 * this->param_.out_2_coefficient_ * c)) / 2 *
            this->param_.out_2_coefficient_;
        clampf(&out, 0.0f, 1.0f);
        motor_out[i] = out;
      } else {
        float out = fabsf(
            (-b - sqrtf(b * b - 4 * this->param_.out_2_coefficient_ * c)) / 2 *
            this->param_.out_2_coefficient_);
        clampf(&out, 0.0f, 1.0f);
        motor_out[i] = -out;
      }
    }
  }
  return 0;
}
template <typename Motor, typename MotorParam>
bool OmniChassis<Motor, MotorParam>::LimitChassisOutPower(float power_limit,
                                                          float* motor_out,
                                                          float* speed_rpm,
                                                          uint32_t len) {
  if (power_limit < 0.0f) {
    return 0;
  }
  // float sum_motor_out = 0.0f;
  // for (size_t i = 0; i < len; i++) {
  //   sum_motor_out += fabsf(motor_out[i]) * fabsf(speed[i]) * 0.06f;
  // }
  // sum_motor_out += 9.2326f;
  // power_1_ = sum_motor_out;
  // if (sum_motor_out > power_limit) {
  //   for (size_t i = 0; i < len; i++) {
  //     motor_out[i] *= power_limit / sum_motor_out;
  //   }
  // }
  // return true;
  float sum_motor_power = 0.0f;
  float motor_3508_power[len];
  for (size_t i = 0; i < len; i++) {
    motor_3508_power[i] =
        this->param_.toque_coefficient_ * fabsf(motor_out[i]) *
            fabsf(speed_rpm[i]) +
        this->param_.speed_2_coefficient_ * speed_rpm[i] * speed_rpm[i] +
        this->param_.out_2_coefficient_ * motor_out[i] * motor_out[i];
    sum_motor_power += motor_3508_power[i];
  }
  sum_motor_power += this->param_.constant_;
  power_ = sum_motor_power;
  power_1_ = power_limit;
  if (sum_motor_power > power_limit) {
    for (size_t i = 0; i < len; i++) {
      motor_out[i] *= power_limit / sum_motor_power;
    }
  }
  return true;
}
template <typename Motor, typename MotorParam>
void OmniChassis<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  max_motor_rotational_speed_ = this->MAXSPEEDGET(ref_.chassis_power_limit);
  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 计算vx、vy */
  switch (this->mode_) {
    case OmniChassis::BREAK: /* 刹车模式电机停止 */
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;

    case OmniChassis::INDENPENDENT: /* 独立模式控制向量与运动向量相等
                                     */
      this->move_vec_.vx = this->cmd_.x;
      this->move_vec_.vy = this->cmd_.y;
      break;

    case OmniChassis::RELAX:
    case OmniChassis::FOLLOW_GIMBAL_INTERSECT: /* 按照云台方向换算运动向量
                                                */
    case OmniChassis::FOLLOW_GIMBAL_CROSS:
    case OmniChassis::ROTOR: {
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
    case OmniChassis::RELAX:
    case OmniChassis::BREAK:
    case OmniChassis::INDENPENDENT: /* 独立模式wz为0 */
      this->move_vec_.wz = this->cmd_.z;
      break;

    case OmniChassis::FOLLOW_GIMBAL_INTERSECT:
      this->move_vec_.wz =
          this->follow_pid_.Calculate(0.0f, this->yaw_, this->dt_);
      break;
    case OmniChassis::FOLLOW_GIMBAL_CROSS:
      this->move_vec_.wz = this->follow_pid_.Calculate(
          0.0f, this->yaw_ - M_PI / 4.0f, this->dt_);
      break;

    case OmniChassis::ROTOR: { /* 小陀螺模式使底盘以一定速度旋转
                                */
      this->move_vec_.wz =
          this->wz_dir_mult_ * CalcWz(ROTOR_WZ_MIN, ROTOR_WZ_MAX);
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
    case OmniChassis::BREAK:
    case OmniChassis::FOLLOW_GIMBAL_INTERSECT:
    case OmniChassis::FOLLOW_GIMBAL_CROSS:
    case OmniChassis::ROTOR:
    case OmniChassis::INDENPENDENT: /* 独立模式,受PID控制 */ {
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
      float max_power_limit =
          ref_.chassis_power_limit +
          ref_.chassis_power_limit * 0.2 * this->cap_.percentage_;
      for (unsigned i = 0; i < this->mixer_.len_; i++) {
        out_.motor3508_out[i] = this->actuator_[i]->Calculate(
            this->setpoint_.motor_rotational_speed[i] *
                max_motor_rotational_speed_,
            this->motor_[i]->GetSpeed(), this->dt_);
      }
      for (unsigned i = 0; i < this->mixer_.len_; i++) {
        if (cap_.online_) {
          LimitChassisOutPower(max_power_limit, out_.motor3508_out,
                               motor_feedback_.motor_speed, this->mixer_.len_);
          this->motor_[i]->Control(out_.motor3508_out[i]);
        } else {
          LimitChassisOutPower(ref_.chassis_power_limit, out_.motor3508_out,
                               motor_feedback_.motor_speed, this->mixer_.len_);

          this->motor_[i]->Control(out_.motor3508_out[i]);
        }
      }
      break;
    }
    case OmniChassis::RELAX: /* 放松模式,不输出 */
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
void OmniChassis<Motor, MotorParam>::PraseRef() {
  this->ref_.chassis_power_limit =
      this->raw_ref_.robot_status.chassis_power_limit;
  this->ref_.chassis_pwr_buff = this->raw_ref_.power_heat.chassis_pwr_buff;
  this->ref_.chassis_watt = this->raw_ref_.power_heat.chassis_watt;
  this->ref_.status = this->raw_ref_.status;
}

template <typename Motor, typename MotorParam>
float OmniChassis<Motor, MotorParam>::CalcWz(const float LO, const float HI) {
  float wz_vary = fabsf(0.2f * sinf(ROTOR_OMEGA * this->now_)) + LO;
  clampf(&wz_vary, LO, HI);
  return wz_vary;
}

template <typename Motor, typename MotorParam>
void OmniChassis<Motor, MotorParam>::SetMode(OmniChassis::Mode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }

  if (mode == OmniChassis::ROTOR && this->mode_ != OmniChassis::ROTOR) {
    std::srand(this->now_);
    this->wz_dir_mult_ = (std::rand() % 2) ? -1 : 1;
  }
  if (mode == OmniChassis::FOLLOW_GIMBAL_CROSS &&
      this->mode_ != OmniChassis::FOLLOW_GIMBAL_CROSS) {
    this->param_.type = Component::Mixer::OMNICROSS;
  }
  if (mode == OmniChassis::FOLLOW_GIMBAL_INTERSECT &&
      this->mode_ != OmniChassis::FOLLOW_GIMBAL_INTERSECT) {
    this->param_.type = Component::Mixer::OMNIPLUS;
  }
  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->actuator_[i]->Reset();
  }
  this->mode_ = mode;
}

template <typename Motor, typename MotorParam>
void OmniChassis<Motor, MotorParam>::DrawUIStatic(
    OmniChassis<Motor, MotorParam>* chassis) {
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
    case FOLLOW_GIMBAL_INTERSECT:
    case FOLLOW_GIMBAL_CROSS:
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
void OmniChassis<Motor, MotorParam>::DrawUIDynamic(
    OmniChassis<Motor, MotorParam>* chassis) {
  float box_pos_left = 0.0f, box_pos_right = 0.0f;

  /* 更新底盘模式选择框 */
  switch (chassis->mode_) {
    case FOLLOW_GIMBAL_INTERSECT:
    case FOLLOW_GIMBAL_CROSS:
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
template class Module::OmniChassis<Device::RMMotor, Device::RMMotor::Param>;
