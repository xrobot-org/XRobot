#include "mod_speed_control.hpp"

using namespace Module;

#define MOTOR_MAX_ROTATIONAL_SPEED (12000.0f)

SpeedControl::SpeedControl(Param& param)
    : pid_(param.pid, 500), motor_(param.motor, "motor") {
  /* 使用lambda创建控制线程函数 */
  auto ctrl_thread = [](SpeedControl* speed_ctrl) {
    uint32_t last_online_time = bsp_time_get_ms();
    while (1) {
      /* 获取控制命令 暂时写死*/
      speed_ctrl->speed_setpoint_ = -1.f;

      /* 更新反馈 */
      speed_ctrl->UpdateFeedback();

      /* 控制电机 */
      speed_ctrl->Control();

      /* 等待下一次控制 */
      speed_ctrl->thread_.SleepUntil(2, last_online_time);
    }
  };

  /* 创建线程 */
  this->thread_.Create(ctrl_thread, this, "speed_control",
                       MODULE_LAUNCHER_TASK_STACK_DEPTH,
                       System::Thread::MEDIUM);
}

void SpeedControl::UpdateFeedback() { this->motor_.Update(); }

void SpeedControl::Control() {
  /* 更新时间 */
  this->now_ = bsp_time_get();
  this->dt_ = this->now_ - this->lask_wakeup_;
  this->lask_wakeup_ = this->now_;

  /* 计算PID */
  this->output_ = this->pid_.Calculate(
      this->speed_setpoint_,
      this->motor_.GetSpeed() / MOTOR_MAX_ROTATIONAL_SPEED, this->dt_);

  /* 电机输出 */
  this->motor_.Control(this->output_);
}
