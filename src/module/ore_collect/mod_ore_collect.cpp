#include "mod_ore_collect.hpp"

#include "bsp_gpio.h"
#include "comp_cmd.hpp"
#include "ms.h"

using namespace Module;

OreCollect::OreCollect(Param& param, float control_freq)
    : param_(param),
      x_actr_(param.x_actr, control_freq, 50.0f),
      pitch_actr_(param.pitch_actr, control_freq, 50.0f),
      pitch_1_actr_(param.pitch_1_actr, control_freq, 50.0f),
      yaw_actr_(param.yaw_actr, control_freq, 50.0f),
      z_actr_(param.z_actr, control_freq, 50.0f),
      z_1_actr_(param.z_1_actr, control_freq, 50.0f),
      y_actr_(param.y_actr, control_freq, 50.0f) {
  auto event_callback = [](Event event, OreCollect* ore) {
    switch (event) {
      case START:
        bsp_gpio_write_pin(BSP_GPIO_SWITCH, true);
        break;
      case STOP:
        bsp_gpio_write_pin(BSP_GPIO_SWITCH, false);
        break;
      case RESET:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 1.0f;
        ore->setpoint_.pitch_1 = 0.28f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.z = 0.0f;
        ore->setpoint_.z_1 = 0.0f;
        ore->setpoint_.y = 0.2f;
        break;
      case STEP_1:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 0.0f;
        ore->setpoint_.pitch_1 = 0.0f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.z = 0.9f;
        ore->setpoint_.z_1 = 0.24f;
        ore->setpoint_.y = 0.0f;
        break;
      case STEP_2:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 0.0f;
        ore->setpoint_.pitch_1 = 0.0f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.z = 0.8f;
        ore->setpoint_.z_1 = 0.12f;
        ore->setpoint_.y = 0.0f;
        break;
      case STEP_3:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 1.0f;
        ore->setpoint_.pitch_1 = 0.62f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.z = 0.9f;
        ore->setpoint_.z_1 = 0.24f;
        ore->setpoint_.y = 0.0f;
        break;
      case STEP_4:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 0.3f;
        ore->setpoint_.pitch_1 = 0.285f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.z = 0.7f;
        ore->setpoint_.z_1 = 0.1f;
        ore->setpoint_.y = 0.0f;
        break;
      case STEP_5:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 0.3f;
        ore->setpoint_.pitch_1 = 0.285f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.z = 0.7f;
        ore->setpoint_.z_1 = 0.1f;
        ore->setpoint_.y = 0.3f;
        break;
      case STEP_6:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 1.0f;
        ore->setpoint_.pitch_1 = 0.62f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.y = 0.5f;
        break;
      case STEP_7:
        ore->setpoint_.z = 0.3f;
        ore->setpoint_.z_1 = 0.0f;
        break;
      case STEP_8:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 0.3f;
        ore->setpoint_.pitch_1 = 0.285f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.z = 0.8f;
        ore->setpoint_.z_1 = 0.5f;
        ore->setpoint_.y = 0.1f;
        break;
      case STEP_9:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 0.3f;
        ore->setpoint_.pitch_1 = 0.285f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.z = 0.8f;
        ore->setpoint_.z_1 = 0.5f;
        ore->setpoint_.y = 0.9f;
        break;
      case STEP_10:
        ore->setpoint_.x = 0.5f;
        ore->setpoint_.pitch = 1.0f;
        ore->setpoint_.pitch_1 = 0.63f;
        ore->setpoint_.yaw = 0.5f;
        ore->setpoint_.z = 0.0f;
        ore->setpoint_.z_1 = 0.0f;
        ore->setpoint_.y = 0.63f;
        break;
    }
  };

  Component::CMD::RegisterEvent<OreCollect*, Event>(event_callback, this,
                                                    this->param_.EVENT_MAP);

  auto chassis_thread = [](OreCollect* ore) {
    while (1) {
      /* 更新反馈值 */
      ore->UpdateFeedback();
      ore->Control();

      /* 运行结束，等待下一次唤醒 */
      ore->thread_.SleepUntil(2);
    }
  };

  this->setpoint_.yaw = 0.5;
  this->setpoint_.pitch = 1.0;
  this->setpoint_.pitch_1 = 0.2;
  this->setpoint_.x = 0.5;
  this->setpoint_.y = 0.2;

  this->thread_.Create(chassis_thread, this, "chassis_thread",
                       MODULE_ORE_TASK_STACK_DEPTH, System::Thread::MEDIUM);
}

void OreCollect::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = this->now_ - this->last_wakeup_;
  this->last_wakeup_ = this->now_;

  // this->y_actr_.Relax();

  this->y_actr_.Control(setpoint_.y * this->param_.y_actr.max_range, dt_);

  this->z_actr_.Control(setpoint_.z * this->param_.z_actr.max_range, dt_);

  this->z_1_actr_.Control(setpoint_.z_1 * this->param_.z_1_actr.max_range, dt_);

  this->pitch_1_actr_.Control(
      setpoint_.pitch_1 * this->param_.pitch_1_actr.max_range, dt_);

  if (pitch_1_actr_.Ready()) {
    this->x_actr_.Control(setpoint_.x * this->param_.x_actr.max_range, dt_);
  } else {
    this->x_actr_.Relax();
  }

  this->pitch_actr_.Control(setpoint_.pitch * this->param_.pitch_actr.max_range,
                            dt_);

  if (pitch_actr_.Ready()) {
    this->yaw_actr_.Control(setpoint_.yaw * this->param_.yaw_actr.max_range,
                            dt_);
  } else {
    yaw_actr_.Relax();
  }
}

void OreCollect::UpdateFeedback() {
  this->x_actr_.UpdateFeedback();
  this->pitch_actr_.UpdateFeedback();
  this->pitch_1_actr_.UpdateFeedback();
  this->yaw_actr_.UpdateFeedback();
  this->z_actr_.UpdateFeedback();
  this->z_1_actr_.UpdateFeedback();
  this->y_actr_.UpdateFeedback();
}
