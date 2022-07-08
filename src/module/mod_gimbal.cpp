/*
 * 云台模组
 */

#include "mod_gimbal.hpp"

#include <stdlib.h>

using namespace Module;

Gimbal::Gimbal(Param& param, float control_freq)
    : param_(param),
      mode_(Component::CMD::GIMBAL_MODE_RELAX),
      st_(param.st),
      yaw_actuator_(this->param_.yaw, control_freq, "gimbal_yaw"),
      pit_actuator_(this->param_.ff, this->param_.pit, control_freq,
                    "gimbal_pit") {
  auto gimbal_thread = [](void* arg) {
    Gimbal* gimbal = (Gimbal*)arg;

    DECLARE_TOPIC(ui_tp, gimbal->ui_, "gimbal_ui", true);
    DECLARE_TOPIC(yaw_tp, gimbal->yaw_offset_angle_, "gimbal_yaw_offset", true);

    DECLARE_SUBER(eulr_tp, gimbal->feedback_.imu, "gimbal_eulr");
    DECLARE_SUBER(gyro_tp, gimbal->feedback_.gyro, "gimbal_gyro");
    DECLARE_SUBER(cmd_tp, gimbal->cmd_, "cmd_gimbal");

    while (1) {
      /* 读取控制指令、姿态、IMU、电机反馈 */
      eulr_tp.DumpData();
      gyro_tp.DumpData();
      cmd_tp.DumpData();

      gimbal->UpdateFeedback();
      gimbal->Control();
      gimbal->PackUI();

      ui_tp.Publish();
      yaw_tp.Publish();

      /* 运行结束，等待下一次唤醒 */
      gimbal->thread_.Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, gimbal_thread, 512, System::Thread::Medium,
                 this);
}

void Gimbal::UpdateFeedback() {
  this->pit_actuator_.UpdateFeedback();
  this->yaw_actuator_.UpdateFeedback();

  this->yaw_offset_angle_ =
      circle_error(this->yaw_actuator_.motor_.feedback_.rotor_abs_angle,
                   this->param_.mech_zero.yaw, M_2PI);
}

void Gimbal::Control() {
  this->now_ = System::Thread::GetTick();
  this->dt_ = (float)(this->now_ - this->lask_wakeup_) / 1000.0f;
  this->lask_wakeup_ = this->now_;

  SetMode(this->cmd_.mode);

  /* yaw坐标正方向与遥控器操作逻辑相反 */
  this->cmd_.delta_eulr.pit = this->cmd_.delta_eulr.pit;
  this->cmd_.delta_eulr.yaw = -this->cmd_.delta_eulr.yaw;

  switch (this->mode_) {
#if CTRL_MODE_AUTO
    case Component::CMD::GIMBAL_MODE_SCAN: {
      /* 判断YAW轴运动方向 */
      const float yaw_offset = circle_error(this->feedback_.imu.yaw, 0, M_2PI);

      if (fabs(yaw_offset) > ANGLE2RANDIAN(GIM_SCAN_ANGLE_BASE)) {
        if (yaw_offset > 0)
          this->scan_yaw_direction_ = -1;
        else {
          this->scan_yaw_direction_ = 1;
        }
      }

      /* 判断PIT轴运动方向 */
      if (circle_error(this->pit_actuator_.GetPos(),
                       this->param_.limit.pitch_min,
                       M_2PI) <= ANGLE2RANDIAN(GIM_SCAN_CRITICAL_ERR)) {
        this->scan_pit_direction_ = 1;
      } else if (circle_error(this->param_.limit.pitch_max,
                              this->pit_actuator_.GetPos(),
                              M_2PI) <= ANGLE2RANDIAN(GIM_SCAN_CRITICAL_ERR)) {
        this->scan_pit_direction_ = -1;
      }

      /* 覆盖控制命令 */
      this->cmd_.delta_eulr.yaw = SPEED2DELTA(GIM_SCAN_YAW_SPEED, this->dt_) *
                                  this->scan_yaw_direction_;
      this->cmd_.delta_eulr.pit = SPEED2DELTA(GIM_SCAN_PIT_SPEED, this->dt_) *
                                  this->scan_pit_direction_;

      break;
    }
#endif
    default:
      break;
  }

  /* 处理yaw控制命令 */
  circle_add(&(this->setpoint.eulr_.yaw), this->cmd_.delta_eulr.yaw, M_2PI);

  /* 处理pitch控制命令，软件限位 */
  const float delta_max =
      circle_error(this->param_.limit.pitch_max,
                   (this->pit_actuator_.GetPos() + this->setpoint.eulr_.pit -
                    this->feedback_.imu.pit),
                   M_2PI);
  const float delta_min =
      circle_error(this->param_.limit.pitch_min,
                   (this->pit_actuator_.GetPos() + this->setpoint.eulr_.pit -
                    this->feedback_.imu.pit),
                   M_2PI);
  clampf(&(this->cmd_.delta_eulr.pit), delta_min, delta_max);
  this->setpoint.eulr_.pit += this->cmd_.delta_eulr.pit;

  /* 控制相关逻辑 */
  switch (this->mode_) {
    case Component::CMD::GIMBAL_MODE_RELAX:
    case Component::CMD::GIMBAL_MODE_RELATIVE:
      this->yaw_actuator_.Relax();
      this->pit_actuator_.Relax();
      break;
    case Component::CMD::GIMBAL_MODE_SCAN:
    case Component::CMD::GIMBAL_MODE_ABSOLUTE:
      /* Yaw轴角速度环参数计算 */
      float yaw_k = this->st_.GetValue(this->feedback_.imu.pit);
      this->yaw_actuator_.speed_->SetK(yaw_k);

      this->yaw_actuator_.Control(this->setpoint.eulr_.yaw,
                                  this->feedback_.imu.yaw,
                                  this->feedback_.gyro.z, this->dt_);

      this->pit_actuator_.Control(
          this->setpoint.eulr_.pit, this->feedback_.imu.pit,
          this->feedback_.gyro.x, this->feedback_.imu.pit, this->dt_);

      break;
  }
}

void Gimbal::PackUI() { this->ui_.mode = this->mode_; }

void Gimbal::SetMode(Component::CMD::GimbalMode mode) {
  if (mode == this->mode_) return;

  /* 切换模式后重置PID和滤波器 */
  this->pit_actuator_.Reset();
  this->yaw_actuator_.Reset();

  memcpy(&(this->setpoint.eulr_), &(this->feedback_.imu),
         sizeof(this->setpoint.eulr_)); /* 切换模式后重置设定值 */
  if (this->mode_ == Component::CMD::GIMBAL_MODE_RELAX) {
    if (mode == Component::CMD::GIMBAL_MODE_ABSOLUTE) {
      this->setpoint.eulr_.yaw = this->feedback_.imu.yaw;
    } else if (mode == Component::CMD::GIMBAL_MODE_RELATIVE) {
      this->setpoint.eulr_.yaw = this->yaw_actuator_.GetPos();
    }
  }

  if (mode == Component::CMD::GIMBAL_MODE_SCAN) {
    this->scan_pit_direction_ = (rand() % 2) ? -1 : 1;
    this->scan_yaw_direction_ = (rand() % 2) ? -1 : 1;
  }

  this->mode_ = mode;

  memcpy(&(this->setpoint.eulr_), &(this->feedback_.imu),
         sizeof(this->setpoint.eulr_)); /* 切换模式后重置设定值 */
  if (this->mode_ == Component::CMD::GIMBAL_MODE_RELAX) {
    if (mode == Component::CMD::GIMBAL_MODE_ABSOLUTE) {
      this->setpoint.eulr_.yaw = this->feedback_.imu.yaw;
    } else if (mode == Component::CMD::GIMBAL_MODE_RELATIVE) {
      this->setpoint.eulr_.yaw = this->yaw_actuator_.GetPos();
    }
  }

  if (mode == Component::CMD::GIMBAL_MODE_SCAN) {
    this->scan_pit_direction_ = (rand() % 2) ? -1 : 1;
    this->scan_yaw_direction_ = (rand() % 2) ? -1 : 1;
  }

  this->mode_ = mode;
}
