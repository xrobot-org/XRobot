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

#include <stdlib.h>

#include "FreeRTOS.h"
#include "dev_can.hpp"
#include "dev_cap.hpp"
#include "dev_tof.hpp"

#define ROTOR_WZ_MIN 0.6f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 0.8f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.0015f /* 小陀螺转动频率 */

#define SCAN_VY_LENGTH_MIN 0.3f /* 哨兵返回时距离边界的最小值 */
#define SCAN_MOVEMENTS 0.6f     /* 哨兵移动速度 */

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

Chassis::Chassis(Param& param, float control_freq)
    : param_(param),
      mode_(Component::CMD::CHASSIS_MODE_RELAX),
      mixer_(param.type),
      follow_pid_(param.follow_pid_param, control_freq) {
  memset(&(this->cmd_), 0, sizeof(this->cmd_));

  for (uint8_t i = 0; i < this->mixer_.len_; i++) {
    this->actuator_[i] = (Device::SpeedActuator*)System::Memory::Malloc(
        sizeof(Device::SpeedActuator));
    new (this->actuator_[i])
        Device::SpeedActuator(param.actuator_param[i], control_freq);

    this->motor_[i] =
        (Device::RMMotor*)System::Memory::Malloc(sizeof(Device::RMMotor));
    new (this->motor_[i])
        Device::RMMotor(param.motor_param[i],
                        (std::string("chassis_") + std::to_string(i)).c_str());
  }

  this->setpoint.motor_rotational_speed = (float*)System::Memory::Malloc(
      this->mixer_.len_ * sizeof(*this->setpoint.motor_rotational_speed));
  ASSERT(this->setpoint.motor_rotational_speed);

  auto chassis_thread = [](void* arg) {
    Chassis* chassis = (Chassis*)arg;

    Device::Referee::Data ref_raw;

    DECLARE_TOPIC(cap_out_tp, chassis->cap_control_, "cap_out", true);
    DECLARE_TOPIC(ui_tp, chassis->ui_, "chassis_ui", true);

    DECLARE_SUBER(ref_tp, ref_raw, "referee");
    DECLARE_SUBER(cap_info_tp, chassis->cap_fb_, "cap_info");
    DECLARE_SUBER(yaw_tp, chassis->gimbal_yaw_offset, "gimbal_yaw_offset");
    DECLARE_SUBER(cmd_tp, chassis->cmd_, "cmd_chassis");

#if ID_SENTRY
    DECLARE_SUBER(tof_tp, chassis->tof_fb_, "tof_fb");
#endif

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      yaw_tp.DumpData();
      ref_tp.DumpData();
      cmd_tp.DumpData();
      cap_info_tp.DumpData();
#if ID_SENTRY
      tof_tp.DumpData();
#endif
      /* 更新反馈值 */
      chassis->PraseRef(ref_raw);
      chassis->UpdateFeedback();
      chassis->Control();
      chassis->PackUI();

      cap_out_tp.Publish();
      ui_tp.Publish();

      /* 运行结束，等待下一次唤醒 */
      chassis->thread_.Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, chassis_thread, 384, System::Thread::Medium,
                 this);
}

void Chassis::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->motor_[i]->Update();
  }
}

void Chassis::Control() {
  this->now_ = System::Thread::GetTick();

  this->dt_ = (float)(this->now_ - this->last_wakeup_) / 1000.0f;
  this->last_wakeup_ = this->now_;

  /* 根据遥控器命令更改底盘模式 */
  this->SetMode(cmd_.mode);
  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 计算vx、vy */
  switch (this->mode_) {
    case Component::CMD::CHASSIS_MODE_BREAK: /* 刹车模式电机停止 */
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;

    case Component::CMD::
        CHASSIS_MODE_INDENPENDENT: /* 独立模式控制向量与运动向量相等
                                    */
      this->move_vec_.vx = this->cmd_.ctrl_vec.vx;
      this->move_vec_.vy = this->cmd_.ctrl_vec.vy;
      break;

    case Component::CMD::CHASSIS_MODE_RELAX:
    case Component::CMD::CHASSIS_MODE_FOLLOW_GIMBAL: /* 按照云台方向换算运动向量
                                                      */
    case Component::CMD::CHASSIS_MODE_ROTOR: {
      float beta = this->gimbal_yaw_offset;
      float cos_beta = cosf(beta);
      float sin_beta = sinf(beta);
      this->move_vec_.vx =
          cos_beta * this->cmd_.ctrl_vec.vx - sin_beta * this->cmd_.ctrl_vec.vy;
      this->move_vec_.vy =
          sin_beta * this->cmd_.ctrl_vec.vx + cos_beta * this->cmd_.ctrl_vec.vy;
      break;
    }
#if ID_SENTRY
    case Component::CMD::CHASSIS_MODE_SCAN:
      /* 根据距离传感器数据变向 */
      if (this->tof_fb_[Device::Tof::DEV_TOF_SENSOR_LEFT].dist <
          SCAN_VY_LENGTH_MIN)
        this->vy_dir_mult_ = 1;
      else if (this->tof_fb_[Device::Tof::DEV_TOF_SENSOR_RIGHT].dist <
               SCAN_VY_LENGTH_MIN)
        this->vy_dir_mult_ = -1;
      this->move_vec_.vy = this->vy_dir_mult_ * SCAN_MOVEMENTS;
      break;
#endif
    default:
      break;
  }

  /* 计算wz */
  switch (this->mode_) {
    case Component::CMD::CHASSIS_MODE_RELAX:
    case Component::CMD::CHASSIS_MODE_BREAK:
    case Component::CMD::CHASSIS_MODE_INDENPENDENT: /* 独立模式wz为0 */
      this->move_vec_.wz = 0.0f;
      break;

    case Component::CMD::
        CHASSIS_MODE_FOLLOW_GIMBAL: /* 跟随模式通过PID控制使车头跟随云台
                                     */
      this->move_vec_.wz =
          this->follow_pid_.Calculate(0.0f, this->gimbal_yaw_offset, this->dt_);
      break;

    case Component::CMD::CHASSIS_MODE_ROTOR: { /* 小陀螺模式使底盘以一定速度旋转
                                                */
      this->move_vec_.wz =
          this->wz_dir_mult_ * CalcWz(ROTOR_WZ_MIN, ROTOR_WZ_MAX);
      break;
    }
    case Component::CMD::CHASSIS_MODE_SCAN:
      this->move_vec_.wz = 0;
      break;
    default:
      ASSERT(false);
      return;
  }

  /* move_vec -> motor_rpm_set. 通过运动向量计算轮子转速目标值 */
  this->mixer_.Apply(this->move_vec_, this->setpoint.motor_rotational_speed);

  /* 根据轮子转速目标值，利用PID计算电机输出值 */

  /* 根据底盘模式计算输出值 */
  switch (this->mode_) {
    case Component::CMD::CHASSIS_MODE_BREAK:
    case Component::CMD::CHASSIS_MODE_FOLLOW_GIMBAL:
    case Component::CMD::CHASSIS_MODE_ROTOR:
    case Component::CMD::CHASSIS_MODE_SCAN:
    case Component::CMD::CHASSIS_MODE_INDENPENDENT: /* 独立模式,受PID控制 */ {
      float buff_percentage = this->ref_.chassis_pwr_buff / 30.0f;
      clampf(&buff_percentage, 0.0f, 1.0f);
      for (unsigned i = 0; i < this->mixer_.len_; i++) {
        float out = this->actuator_[i]->Calculation(
            this->setpoint.motor_rotational_speed[i] *
                MOTOR_MAX_ROTATIONAL_SPEED,
            this->motor_[i]->GetSpeed(), this->dt_);

        this->motor_[i]->Control(out);
      }

      break;
    }
    case Component::CMD::CHASSIS_MODE_RELAX: /* 放松模式,不输出 */
      for (size_t i = 0; i < this->mixer_.len_; i++) {
        this->motor_[i]->Relax();
      }
      break;
    default:
      ASSERT(false);
      return;
  }
}

void Chassis::PackUI() {
  this->ui_.mode = this->mode_;
  this->ui_.angle = this->gimbal_yaw_offset;
}

void Chassis::PraseRef(Device::Referee::Data& ref) {
  this->ref_.chassis_power_limit = ref.robot_status.chassis_power_limit;
  this->ref_.chassis_pwr_buff = ref.power_heat.chassis_pwr_buff;
  this->ref_.chassis_watt = ref.power_heat.chassis_watt;
  this->ref_.status = ref.status;
}

float Chassis::CalcWz(const float lo, const float hi) {
  float wz_vary = fabsf(0.2f * sinf(ROTOR_OMEGA * (float)this->now_)) + lo;
  clampf(&wz_vary, lo, hi);
  return wz_vary;
}

void Chassis::SetMode(Component::CMD::ChassisMode mode) {
  if (mode == this->mode_) return; /* 模式未改变直接返回 */

  if (mode == Component::CMD::CHASSIS_MODE_SCAN &&
      this->mode_ != Component::CMD::CHASSIS_MODE_SCAN) {
    this->vy_dir_mult_ = (rand() % 2) ? -1 : 1;
  }

  if (mode == Component::CMD::CHASSIS_MODE_ROTOR &&
      this->mode_ != Component::CMD::CHASSIS_MODE_ROTOR) {
    srand(this->now_);
    this->wz_dir_mult_ = (rand() % 2) ? -1 : 1;
  }
  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->actuator_[i]->Reset();
  }
  this->mode_ = mode;
}
