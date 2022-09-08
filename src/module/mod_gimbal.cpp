/*
 * 云台模组
 */

#include "mod_gimbal.hpp"

#include <stdlib.h>

using namespace Module;

#define GIMBAL_MAX_SPEED (M_2PI * 1.5f)

Gimbal::Gimbal(Param& param, float control_freq)
    : param_(param),
      st_(param.st),
      yaw_actuator_(this->param_.yaw_actr, control_freq),
      pit_actuator_(this->param_.pit_actr, control_freq),
      yaw_motor_(this->param_.yaw_motor, "gimbal_yaw"),
      pit_motor_(this->param_.pit_motor, "gimbal_pit"),
      ctrl_lock_(true) {
  auto event_callback = [](uint32_t event, void* arg) {
    Gimbal* gimbal = static_cast<Gimbal*>(arg);

    gimbal->ctrl_lock_.Take(UINT32_MAX);

    gimbal->SetMode((Mode)event);

    gimbal->ctrl_lock_.Give();
  };

  Component::CMD::RegisterEvent(event_callback, this, this->param_.event_map);

  auto gimbal_thread = [](void* arg) {
    Gimbal* gimbal = (Gimbal*)arg;

    DECLARE_SUBER(eulr_, gimbal->eulr_, "imu_eulr");
    DECLARE_SUBER(gyro_, gimbal->gyro_, "imu_gyro");
    DECLARE_SUBER(cmd_, gimbal->cmd_, "cmd_gimbal");

    while (1) {
      /* 读取控制指令、姿态、IMU、电机反馈 */
      eulr_.DumpData();
      gyro_.DumpData();
      cmd_.DumpData();

      gimbal->ctrl_lock_.Take(UINT32_MAX);
      gimbal->UpdateFeedback();
      gimbal->Control();
      gimbal->ctrl_lock_.Give();

      gimbal->yaw_.Publish();

      /* 运行结束，等待下一次唤醒 */
      gimbal->thread_.Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, gimbal_thread, 512, System::Thread::Medium,
                 this);
}

void Gimbal::UpdateFeedback() {
  this->pit_motor_.Update();
  this->yaw_motor_.Update();

  this->yaw_.data_ = circle_error(this->yaw_motor_.GetAngle(),
                                  this->param_.mech_zero.yaw, M_2PI);
}

void Gimbal::Control() {
  this->now_ = System::Thread::GetTick();
  this->dt_ = (float)(this->now_ - this->lask_wakeup_) / 1000.0f;
  this->lask_wakeup_ = this->now_;

  /* yaw坐标正方向与遥控器操作逻辑相反 */
  float gimbal_pit_cmd = this->cmd_.eulr.pit * this->dt_ * GIMBAL_MAX_SPEED;
  float gimbal_yaw_cmd = -this->cmd_.eulr.yaw * this->dt_ * GIMBAL_MAX_SPEED;

  switch (this->mode_) {
#if CTRL_MODE_AUTO
    case Component::CMD::Scan: {
      /* 判断YAW轴运动方向 */
      const float yaw_offset = circle_error(this->eulr_..yaw, 0, M_2PI);

      if (fabs(yaw_offset) > ANGLE2RANDIAN(GIM_SCAN_ANGLE_BASE)) {
        if (yaw_offset > 0)
          this->scan_yaw_direction_ = -1;
        else {
          this->scan_yaw_direction_ = 1;
        }
      }

      /* 判断PIT轴运动方向 */
      if (circle_error(this->pit_actuator_.GetAngle(),
                       this->param_.limit.pitch_min,
                       M_2PI) <= ANGLE2RANDIAN(GIM_SCAN_CRITICAL_ERR)) {
        this->scan_pit_direction_ = 1;
      } else if (circle_error(this->param_.limit.pitch_max,
                              this->pit_actuator_.GetAngle(),
                              M_2PI) <= ANGLE2RANDIAN(GIM_SCAN_CRITICAL_ERR)) {
        this->scan_pit_direction_ = -1;
      }

      /* 覆盖控制命令 */
      gimbal_yaw_cmd = SPEED2DELTA(GIM_SCAN_YAW_SPEED, this->dt_) *
                       this->scan_yaw_direction_;
      gimbal_pit_cmd = SPEED2DELTA(GIM_SCAN_PIT_SPEED, this->dt_) *
                       this->scan_pit_direction_;

      break;
    }
#endif
    default:
      break;
  }

  /* 处理yaw控制命令 */
  circle_add(&(this->setpoint.eulr_.yaw), gimbal_yaw_cmd, M_2PI);

  /* 处理pitch控制命令，软件限位 */
  const float delta_max =
      circle_error(this->param_.limit.pitch_max,
                   (this->pit_motor_.GetAngle() + this->setpoint.eulr_.pit -
                    this->eulr_.pit),
                   M_2PI);
  const float delta_min =
      circle_error(this->param_.limit.pitch_min,
                   (this->pit_motor_.GetAngle() + this->setpoint.eulr_.pit -
                    this->eulr_.pit),
                   M_2PI);
  clampf(&(gimbal_pit_cmd), delta_min, delta_max);
  circle_add(&(this->setpoint.eulr_.pit), gimbal_pit_cmd, M_2PI);

  /* 控制相关逻辑 */
  switch (this->mode_) {
    case Relax:
      this->yaw_motor_.Relax();
      this->pit_motor_.Relax();
      break;
    case Scan:
    case Absolute:
      /* Yaw轴角速度环参数计算 */
      float yaw_out = this->yaw_actuator_.Calculation(
          this->setpoint.eulr_.yaw, this->gyro_.z, this->eulr_.yaw, this->dt_);

      float pit_out = this->pit_actuator_.Calculation(
          this->setpoint.eulr_.pit, this->gyro_.x, this->eulr_.pit, this->dt_);

      this->yaw_motor_.Control(yaw_out);
      this->pit_motor_.Control(pit_out);

      break;
  }
}

void Gimbal::SetMode(Mode mode) {
  if (mode == this->mode_) return;

  /* 切换模式后重置PID和滤波器 */
  this->pit_actuator_.Reset();
  this->yaw_actuator_.Reset();

  memcpy(&(this->setpoint.eulr_), &(this->eulr_),
         sizeof(this->setpoint.eulr_)); /* 切换模式后重置设定值 */
  if (this->mode_ == Relax) {
    if (mode == Absolute) {
      this->setpoint.eulr_.yaw = this->eulr_.yaw;
    }
  }

  if (mode == Scan) {
    this->scan_pit_direction_ = (rand() % 2) ? -1 : 1;
    this->scan_yaw_direction_ = (rand() % 2) ? -1 : 1;
  }

  this->mode_ = mode;

  memcpy(&(this->setpoint.eulr_), &(this->eulr_),
         sizeof(this->setpoint.eulr_)); /* 切换模式后重置设定值 */
  if (this->mode_ == Relax) {
    if (mode == Absolute) {
      this->setpoint.eulr_.yaw = this->eulr_.yaw;
    }
  }

  if (mode == Scan) {
    this->scan_pit_direction_ = (rand() % 2) ? -1 : 1;
    this->scan_yaw_direction_ = (rand() % 2) ? -1 : 1;
  }

  this->mode_ = mode;
}
