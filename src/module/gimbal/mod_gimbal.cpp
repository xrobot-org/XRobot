/*
 * 云台模组
 */

#include "mod_gimbal.hpp"

#include "bsp_time.h"

using namespace Module;

#define GIMBAL_MAX_SPEED (M_2PI * 1.5f)

Gimbal::Gimbal(Param& param, float control_freq)
    : param_(param),
      st_(param.st),
      yaw_actuator_(this->param_.yaw_actr, control_freq),
      pit_actuator_(this->param_.pit_actr, control_freq),
      yaw_motor_(this->param_.yaw_motor, "Gimbal_Yaw"),
      pit_motor_(this->param_.pit_motor, "Gimbal_Pitch"),
      ctrl_lock_(true) {
  auto event_callback = [](uint32_t event, void* arg) {
    Gimbal* gimbal = static_cast<Gimbal*>(arg);

    gimbal->ctrl_lock_.Take(UINT32_MAX);

    gimbal->SetMode(static_cast<Mode>(event));

    gimbal->ctrl_lock_.Give();
  };

  Component::CMD::RegisterEvent(event_callback, this, this->param_.EVENT_MAP);

  auto gimbal_thread = [](Gimbal* gimbal) {
    auto eulr_sub = Message::Subscriber("imu_eulr", gimbal->eulr_);

    auto gyro_sub = Message::Subscriber("imu_gyro", gimbal->gyro_);

    auto cmd_sub = Message::Subscriber("cmd_gimbal", gimbal->cmd_);

    while (1) {
      /* 读取控制指令、姿态、IMU、电机反馈 */
      eulr_sub.DumpData();
      gyro_sub.DumpData();
      cmd_sub.DumpData();

      gimbal->ctrl_lock_.Take(UINT32_MAX);
      gimbal->UpdateFeedback();
      gimbal->Control();
      gimbal->ctrl_lock_.Give();

      gimbal->yaw_tp_.Publish(gimbal->yaw_);

      /* 运行结束，等待下一次唤醒 */
      gimbal->thread_.SleepUntil(2);
    }
  };

  this->thread_.Create(gimbal_thread, this, "gimbal_thread", 512,
                       System::Thread::MEDIUM);
}

void Gimbal::UpdateFeedback() {
  this->pit_motor_.Update();
  this->yaw_motor_.Update();

  this->yaw_ = circle_error(this->yaw_motor_.GetAngle(),
                            this->param_.mech_zero.yaw, M_2PI);
}

void Gimbal::Control() {
  this->now_ = bsp_time_get();
  this->dt_ = this->now_ - this->last_wakeup_;
  this->last_wakeup_ = this->now_;

  /* yaw坐标正方向与遥控器操作逻辑相反 */
  float gimbal_pit_cmd = this->cmd_.eulr.pit * this->dt_ * GIMBAL_MAX_SPEED;
  float gimbal_yaw_cmd = -this->cmd_.eulr.yaw * this->dt_ * GIMBAL_MAX_SPEED;

  /* 处理yaw控制命令 */
  circle_add(&(this->setpoint.eulr_.yaw), gimbal_yaw_cmd, M_2PI);

  /* 处理pitch控制命令，软件限位 */
  const float DELTA_MAX =
      circle_error(this->param_.limit.pitch_max,
                   (this->pit_motor_.GetAngle() + this->setpoint.eulr_.pit -
                    this->eulr_.pit),
                   M_2PI);
  const float DELTA_MIN =
      circle_error(this->param_.limit.pitch_min,
                   (this->pit_motor_.GetAngle() + this->setpoint.eulr_.pit -
                    this->eulr_.pit),
                   M_2PI);
  clampf(&(gimbal_pit_cmd), DELTA_MIN, DELTA_MAX);
  circle_add(&(this->setpoint.eulr_.pit), gimbal_pit_cmd, M_2PI);

  /* 控制相关逻辑 */
  switch (this->mode_) {
    case RELAX:
      this->yaw_motor_.Relax();
      this->pit_motor_.Relax();
      break;
    case ABSOLUTE:
      /* Yaw轴角速度环参数计算 */
      float yaw_out = this->yaw_actuator_.Calculate(
          this->setpoint.eulr_.yaw, this->gyro_.z, this->eulr_.yaw, this->dt_);

      float pit_out = this->pit_actuator_.Calculate(
          this->setpoint.eulr_.pit, this->gyro_.x, this->eulr_.pit, this->dt_);

      this->yaw_motor_.Control(yaw_out);
      this->pit_motor_.Control(pit_out);

      break;
  }
}

void Gimbal::SetMode(Mode mode) {
  if (mode == this->mode_) {
    return;
  }

  /* 切换模式后重置PID和滤波器 */
  this->pit_actuator_.Reset();
  this->yaw_actuator_.Reset();

  memcpy(&(this->setpoint.eulr_), &(this->eulr_),
         sizeof(this->setpoint.eulr_)); /* 切换模式后重置设定值 */
  if (this->mode_ == RELAX) {
    if (mode == ABSOLUTE) {
      this->setpoint.eulr_.yaw = this->eulr_.yaw;
    }
  }

  this->mode_ = mode;

  memcpy(&(this->setpoint.eulr_), &(this->eulr_),
         sizeof(this->setpoint.eulr_)); /* 切换模式后重置设定值 */
  if (this->mode_ == RELAX) {
    if (mode == ABSOLUTE) {
      this->setpoint.eulr_.yaw = this->eulr_.yaw;
    }
  }

  this->mode_ = mode;
}
