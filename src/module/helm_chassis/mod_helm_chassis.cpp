#include "mod_helm_chassis.hpp"

using namespace Module;
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
#include "mod_helm_chassis.hpp"

#define ROTOR_WZ_MIN 0.8f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 1.0f /* 小陀螺旋转位移上界 */

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

#define MOTOR_MAX_SPEED_COFFICIENT 1.2f /* 电机的最大转速 */

// #define MOTOR_ROTATION_MAX_SPEED 9000; /* 电机的最大转速rpm */

using namespace Module;

template <typename Motor, typename MotorParam>
HelmChassis<Motor, MotorParam>::HelmChassis(Param& param, float control_freq)
    : param_(param),
      mode_(HelmChassis::RELAX),
      follow_pid_(param.follow_pid_param, control_freq),
      ctrl_lock_(true) {
  memset(&(this->cmd_), 0, sizeof(this->cmd_));
  for (int i = 0; i < 4; i++) {
    this->pos_actr_.at(i) =
        new Component::PosActuator(param.pos_param.at(i), control_freq);
    this->pos_motor_.at(i) = new Motor(
        param.pos_motor_param.at(i),
        (std::string(string_vector1_[i]) + std::to_string(0)).c_str());
  }

  for (uint8_t i = 0; i < 4; i++) {
    this->speed_actr_.at(i) =
        new Component::SpeedActuator(param.speed_param.at(i), control_freq);

    this->speed_motor_.at(i) = new Motor(
        param.speed_motor_param.at(i),
        (std::string(string_vector2_[i]) + std::to_string(0)).c_str());
  }
  ctrl_lock_.Post();

  auto event_callback = [](ChassisEvent event, HelmChassis* chassis) {
    chassis->ctrl_lock_.Wait(UINT32_MAX);

    switch (event) {
      case SET_MODE_RELAX:
        chassis->SetMode(RELAX);
        break;
      case SET_MODE_6020_FOLLOW:
        chassis->SetMode(CHASSIS_6020_FOLLOW_GIMBAL);
        break;
      case SET_MODE_CHASSIS_FOLLOW:
        chassis->SetMode(CHASSIS_FOLLOW_GIMBAL);
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
    auto pit_sub = Message::Subscriber<float>("chassis_pitch");
    auto alpha_sub = Message::Subscriber<double>("chassis_alpha");
    auto eulr_yaw1_sub = Message::Subscriber<float>("chassis_eulr_yaw1");
    auto tan_pit_sub = Message::Subscriber<double>("chassis_tan_pit");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      cmd_sub.DumpData(chassis->cmd_);
      yaw_sub.DumpData(chassis->yaw_);
      raw_ref_sub.DumpData(chassis->raw_ref_);
      cap_sub.DumpData(chassis->cap_);
      pit_sub.DumpData(chassis->pit_);
      alpha_sub.DumpData(chassis->alpha_);
      eulr_yaw1_sub.DumpData(chassis->eulr_yaw1_);
      tan_pit_sub.DumpData(chassis->tan_pit_);

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
  // this->ref_.chassis_watt = this->raw_ref_.power_heat.chassis_watt;
  this->ref_.status = this->raw_ref_.status;
}
template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < 4; i++) {
    this->pos_motor_[i]->Update();
    this->pos_motor_feedback_[i] = this->pos_motor_[i]->GetSpeed();
    this->speed_motor_[i]->Update();
    this->speed_motor_feedback_[i] = this->speed_motor_[i]->GetSpeed();
  };
  for (int i = 0; i < 4; ++i) {
    this->pos_motor_value_[i] = this->pos_motor_[i]->GetAngle().Value();
    this->speed_motor_value_[i] =
        M_2PI - this->speed_motor_[i]->GetAngle().Value();
    this->pos_err_value_[i] = this->pos_actr_[i]->GetLastError();
  }
  this->wz_test_ = speed_motor_feedback_[0] / 594.04f;
  this->random_ = Getrandom();
  this->test_angle01_ = GetRelateAngle();
  this->test_angle02_ = std::floor(alpha_ / M_PI_4);
}

template <typename Motor, typename MotorParam>
bool HelmChassis<Motor, MotorParam>::LimitChassisOutPower(float power_limit,
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
        this->param_.toque_coefficient_3508 * fabsf(motor_out[i]) *
            fabsf(speed_rpm[i]) +
        this->param_.speed_2_coefficient_3508 * speed_rpm[i] * speed_rpm[i] +
        this->param_.out_2_coefficient_3508 * motor_out[i] * motor_out[i];
    sum_motor_power += motor_power[i];
  }
  sum_motor_power += this->param_.constant_3508;
  power_ = sum_motor_power;
  if (sum_motor_power > power_limit) {
    for (size_t i = 0; i < len; i++) {
      motor_out[i] *= power_limit / sum_motor_power;
    }
  }
  return true;
}

template <typename Motor, typename MotorParam>
uint16_t HelmChassis<Motor, MotorParam>::MAXSPEEDGET(float power_limit) {
  uint16_t speed = 0;
  if (power_limit <= 50.0f) {
    speed = 5000;
  } else if (power_limit <= 60.0f) {
    speed = 6000;
  } else if (power_limit <= 70.0f) {
    speed = 6500;
  } else if (power_limit <= 80.0f) {
    speed = 7000;
  } else if (power_limit <= 100.0f) {
    speed = 7500;
  } else {
    speed = 7000;
  }
  return speed;
}
template <typename Motor, typename MotorParam>
int HelmChassis<Motor, MotorParam>::Getrandom() {
  static int last_random_value = 1;
  if (this->mode_ != ROTOR) {
    this->now_ = bsp_time_get();

    this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

    srand(static_cast<unsigned int>(this->now_));
    int random_value = rand() % 2;
    if (random_value == 0) {
      random_value = -1;
    } else {
      random_value = 1;
    };

    last_random_value = random_value;
  }
  return last_random_value;
}

template <typename Motor, typename MotorParam>
double HelmChassis<Motor, MotorParam>::GetTorqueLength(float angle_a,
                                                       double angle_b,
                                                       int selection) {
  double cofficent = 0, cofficent_1 = 0, cofficent_2 = 0, cofficent_3 = 0,
         cofficent_4 = 0;

  cofficent_1 = sqrt(0.0557 + 0.03 * tan(angle_a) * tan(angle_a) -
                     0.081 * tan(angle_a) * cos(3 * M_PI_4 + angle_b));
  cofficent_2 = sqrt(0.0557 + 0.03 * tan(angle_a) * tan(angle_a) -
                     0.081 * tan(angle_a) * cos(3 * M_PI_4 - angle_b));
  cofficent_3 = sqrt(0.0557 + 0.03 * tan(angle_a) * tan(angle_a) -
                     0.081 * tan(angle_a) * cos(M_PI_4 + angle_b));
  cofficent_4 = sqrt(0.0557 + 0.03 * tan(angle_a) * tan(angle_a) -
                     0.081 * tan(angle_a) * cos(M_PI_4 - angle_b));
  switch (selection) {
    case 0:
      cofficent =
          (1 / ((cofficent_1 / cofficent_2) + (cofficent_1 / cofficent_3) +
                (cofficent_1 / cofficent_4) + 1)) *
          2.8 * sin(pit_);
      break;
    case 1:
      cofficent =
          (1 / ((cofficent_2 / cofficent_1) + (cofficent_2 / cofficent_3) +
                (cofficent_2 / cofficent_4) + 1)) *
          2.8 * sin(pit_);
      break;
    case 2:
      cofficent =
          (1 / ((cofficent_3 / cofficent_1) + (cofficent_3 / cofficent_2) +
                (cofficent_3 / cofficent_4) + 1)) *
          2.8 * sin(pit_);
      break;
    case 3:
      cofficent =
          (1 / ((cofficent_4 / cofficent_1) + (cofficent_4 / cofficent_2) +
                (cofficent_4 / cofficent_3) + 1)) *
          2.8 * sin(pit_);
      break;
    default:
      break;
  }
  return cofficent;
}

template <typename Motor, typename MotorParam>
float HelmChassis<Motor, MotorParam>::GetRelateAngle() {
  static float last_relate_angle = 0;
  if (pit_ < 0.1) {
    float relate_angle = 0;
    relate_angle = this->eulr_yaw1_;

    last_relate_angle = relate_angle;
  }
  return last_relate_angle;
}

template <typename Motor, typename MotorParam>
void HelmChassis<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;
  max_motor_rotational_speed_ = this->MAXSPEEDGET(ref_.chassis_power_limit);

  /* 计算vx、vy */
  switch (this->mode_) {
    case HelmChassis::BREAK: /* 刹车/放松模式电机停止 */
    case HelmChassis::RELAX:
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;
      /* 独立模式控制向量与运动向量相等 */
    case HelmChassis::INDENPENDENT:
    case HelmChassis::CHASSIS_6020_FOLLOW_GIMBAL: {
      // float tmp = 0;
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
    case HelmChassis::CHASSIS_FOLLOW_GIMBAL: {
      float beta = this->yaw_;
      float cos_beta = cosf(beta);
      float sin_beta = sinf(beta);
      this->move_vec_.vx = cos_beta * this->cmd_.x - sin_beta * this->cmd_.y;
      this->move_vec_.vy = sin_beta * this->cmd_.x + cos_beta * this->cmd_.y;
      break;
    }
    case HelmChassis::ROTOR: {
      if (random_ == 1) {
        deviation_angle_value_ =
            this->yaw_ + (0.05 * (speed_motor_feedback_[0] / 594.04f));
      } else {
        deviation_angle_value_ =
            this->yaw_ - (0.054 * (speed_motor_feedback_[0] / 594.04f));
      }
      float beta = deviation_angle_value_;
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
    case HelmChassis::CHASSIS_6020_FOLLOW_GIMBAL:
      /* 跟随模式每个轮子的方向与云台相同 */
      this->move_vec_.wz = 0;
      this->main_direct_ = -yaw_;
      break;
    case HelmChassis::CHASSIS_FOLLOW_GIMBAL:
      this->move_vec_.wz =
          this->follow_pid_.Calculate(0.0f, this->yaw_, this->dt_) * 0.25f;

      break;
    case HelmChassis::ROTOR:
      /* 陀螺模式底盘以一定速度旋转 */
      this->move_vec_.wz = Getrandom();
      // this->wz_dir_mult_ * CalcWz(ROTOR_WZ_MIN, ROTOR_WZ_MAX);
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

    case HelmChassis::CHASSIS_6020_FOLLOW_GIMBAL:
    case HelmChassis::INDENPENDENT: /* 独立模式,受PID控制 */
      for (auto& angle : setpoint_.wheel_pos) {
        angle = main_direct_ + direct_offset_;
      }
      for (auto& speed : setpoint_.motor_rotational_speed) {
        speed = max_motor_rotational_speed_ * move_vec_.vy;
      }
      break;
    case HelmChassis::CHASSIS_FOLLOW_GIMBAL:
    case HelmChassis::ROTOR:
      float x = 0, y = 0, wheel_pos = 0;
      for (int i = 0; i < 4; i++) {
        wheel_pos = -i * M_PI / 2.0f + M_PI / 4.0f * 3.0f;
        x = sinf(wheel_pos) * move_vec_.wz + move_vec_.vx;
        y = cosf(wheel_pos) * move_vec_.wz + move_vec_.vy;
        setpoint_.wheel_pos[i] = -(atan2(y, x) - M_PI / 2.0f);

        if ((move_vec_.vx < 0.05 && move_vec_.vx > -0.05) ||
            (move_vec_.vy < 0.05 && move_vec_.vy > -0.05)) {
          setpoint_.motor_rotational_speed[i] =
              max_motor_rotational_speed_ * sqrt(x * x + y * y);
        } else {
          setpoint_.motor_rotational_speed[i] = max_motor_rotational_speed_ *
                                                sqrt(x * x + y * y) *
                                                (1.41421f / 2.0f);
        }
        // float tmp = setpoint_.motor_rotational_speed[i];
        // clampf(&tmp, -max_motor_rotational_speed_,
        // max_motor_rotational_speed_);
      }
      break;
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

  // //底盘前馈2.0
  // if (pit_ > 0.01) {
  //   forward_coefficient_[0] =
  //       -0.35865 * sin(pit_) * tan(pit_) + 0.913125 * sin(pit_);
  //   forward_coefficient_[1] =
  //       -0.35865 * sin(pit_) * tan(pit_) + 0.913125 * sin(pit_);
  //   forward_coefficient_[2] =
  //       0.35865 * sin(pit_) * tan(pit_) + 0.95542 * sin(pit_);
  //   forward_coefficient_[3] =
  //       0.35865 * sin(pit_) * tan(pit_) + 0.95542 * sin(pit_);
  // } else {
  //   for (float& i : forward_coefficient_) {
  //     i = 0;
  //   }
  // }

  /* 底盘前馈3.0 */

  if (pit_ > 0.04 && mode_ != ROTOR && abs(cmd_.y) > 0.01) {
    constexpr int TERM[8][4] = {{1, 0, 2, 3}, {2, 0, 1, 3}, {3, 1, 0, 2},
                                {3, 2, 0, 1}, {2, 3, 1, 0}, {1, 3, 2, 0},
                                {0, 2, 3, 1}, {0, 1, 3, 2}};
    int state = static_cast<int>(std::floor(alpha_ / M_PI_4));
    switch (state) {
      case 0:  // 1023
        for (int i = 0; i < 4; i++) {
          forward_coefficient_[i] = GetTorqueLength(pit_, alpha_, TERM[0][i]);
        }
        break;
      case 1:  // 2013
        for (int i = 0; i < 4; i++) {
          forward_coefficient_[i] = GetTorqueLength(pit_, alpha_, TERM[1][i]);
        }
        break;
      case 2:  // 3102
        for (int i = 0; i < 4; i++) {
          forward_coefficient_[i] = GetTorqueLength(pit_, alpha_, TERM[2][i]);
        }
        break;
      case 3:  // 3201
        for (int i = 0; i < 4; i++) {
          forward_coefficient_[i] = GetTorqueLength(pit_, alpha_, TERM[3][i]);
        }
        break;
      case 4:  // 2310
        for (int i = 0; i < 4; i++) {
          forward_coefficient_[i] = GetTorqueLength(pit_, alpha_, TERM[4][i]);
        }
        break;
      case 5:  // 1320
        for (int i = 0; i < 4; i++) {
          forward_coefficient_[i] = GetTorqueLength(pit_, alpha_, TERM[5][i]);
        }
        break;
      case 6:  // 0231
        for (int i = 0; i < 4; i++) {
          forward_coefficient_[i] = GetTorqueLength(pit_, alpha_, TERM[6][i]);
        }
        break;
      case 7:  // 0132
        for (int i = 0; i < 4; i++) {
          forward_coefficient_[i] = GetTorqueLength(pit_, alpha_, TERM[7][i]);
        }
        break;
      default:
        break;
    }
  } else {
    for (float& i : forward_coefficient_) {
      i = 0;
    }
  }

  if ((tan_pit_ < 0 && cmd_.y > 0) || (tan_pit_ > 0 && cmd_.y < 0)) {
    for (float& i : forward_coefficient_) {
      i = -i;
    }
  }

  /* 输出计算 */
  for (int i = 0; i < 4; i++) {
    if (motor_reverse_[i]) {
      out_.speed_motor_out[i] =
          speed_actr_[i]->Calculate(-setpoint_.motor_rotational_speed[i],
                                    speed_motor_[i]->GetSpeed(), dt_) -
          forward_coefficient_[i];
      out_.motor6020_out[i] = pos_actr_[i]->Calculate(
          setpoint_.wheel_pos[i] + M_PI + this->param_.mech_zero[i],
          pos_motor_[i]->GetSpeed(), pos_motor_[i]->GetAngle(), dt_);
    } else {
      out_.speed_motor_out[i] =
          speed_actr_[i]->Calculate(setpoint_.motor_rotational_speed[i],
                                    speed_motor_[i]->GetSpeed(), dt_) +
          forward_coefficient_[i];
      out_.motor6020_out[i] = pos_actr_[i]->Calculate(
          setpoint_.wheel_pos[i] + this->param_.mech_zero[i],
          pos_motor_[i]->GetSpeed(), pos_motor_[i]->GetAngle(), dt_);
    }
  }

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
  /* 控制 */
  // float max_power_limit = ref_.chassis_power_limit + ref_.chassis_power_limit
  // *
  //                                                        1.8 *
  //                                                        this->cap_.percentage_;

  // float max_power_limit = ref_.chassis_power_limit;

  // sum_6020_out_ =
  //     Calculate6020Power(out_.motor6020_out, motor_feedback_.pos_speed, 4);
  for (int i = 0; i < 4; i++) {
    if (cap_.online_) {
      LimitChassisOutPower(180.0f, out_.speed_motor_out, speed_motor_feedback_,
                           4);
      this->speed_motor_[i]->Control(out_.speed_motor_out[i]);
      this->pos_motor_[i]->Control(out_.motor6020_out[i]);
    } else {
      LimitChassisOutPower(500.f, out_.speed_motor_out, speed_motor_feedback_,
                           4);
      this->speed_motor_[i]->Control(out_.speed_motor_out[i]);
      this->pos_motor_[i]->Control(out_.motor6020_out[i]);
    }
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
    case CHASSIS_6020_FOLLOW_GIMBAL:
    case CHASSIS_FOLLOW_GIMBAL:
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
    case CHASSIS_6020_FOLLOW_GIMBAL:
    case CHASSIS_FOLLOW_GIMBAL:
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
