#include "mod_ore_collect.hpp"

#include "bsp_gpio.h"
#include "bsp_time.h"
#include "comp_cmd.hpp"

using namespace Module;

Device::PosStream ps = {
    .target_angle_ = {0.0, 0.0f / 180.0f * M_PI, 0.0},
    .target_pos_ = {0.0, 0.05, 0.2},
    .angle_ = {0.0, 0.0, 0.0},
};

OreCollect::OreCollect(Param& param, float control_freq)
    : ctrl_lock_(true),
      param_(param),
      x_actr_(param_.x_actr, control_freq),
      pitch_actr_(param.pitch_actr, control_freq),
      pitch_1_actr_(param.pitch_1_actr, control_freq),
      yaw_actr_(param.yaw_actr, control_freq),
      roll_actr_(param.roll_actr, control_freq),
      y_actr_(param.y_actr, control_freq),
      z_actr_(param.z_actr, control_freq),
      z_1_actr_(param.z_1_actr, control_freq) {
  auto event_callback = [](Collect_Event event, OreCollect* ore) {
    switch (event) {
      case START_VACUUM:
        bsp_gpio_write_pin(BSP_GPIO_SWITCH, true);
        break;
      case STOP_VACUUM:
        bsp_gpio_write_pin(BSP_GPIO_SWITCH, false);
        break;
      case RESET:
        ore->mode_ = RELAX;
        break;
      case FOLD:
        ore->mode_ = CALI;
        ore->setpoint_.x = 0.13f;
        ore->setpoint_.pitch = 0.0f;
        ore->setpoint_.pitch_1 = 1.5f;
        ore->setpoint_.yaw = 0.0f;
        ore->setpoint_.roll = 0.0f;
        ore->setpoint_.y = 0.02f;
        ore->setpoint_.z = 0.0f;
        ore->setpoint_.z_1 = 0.0f;
        break;
      case WORK:
        ore->mode_ = MOVE;
      case START_AUTO_COLLECT:
        Component::CMD::SetCtrlSource(Component::CMD::CTRL_SOURCE_AI);
      case STOP_AUTO_COLLECT:
        Component::CMD::SetCtrlSource(Component::CMD::CTRL_SOURCE_EXT);
        break;
    }
  };

  Component::CMD::RegisterEvent<OreCollect*, Collect_Event>(
      event_callback, this, this->param_.EVENT_MAP);

  auto ore_thread = [](OreCollect* ore) {
    auto eulr_sub =
        Message::Subscriber<Component::Type::Eulr>("custom_ctrl_eulr");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      /* 更新反馈值 */
      eulr_sub.DumpData(ore->eulr_);
      ore->UpdateFeedback();
      ore->Control();

      /* 运行结束，等待下一次唤醒 */
      ore->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(ore_thread, this, "ore_thread",
                       MODULE_ORE_TASK_STACK_DEPTH, System::Thread::MEDIUM);
}

void OreCollect::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  switch (mode_) {
    case RELAX:
      x_actr_.Relax();
      pitch_actr_.Relax();
      pitch_1_actr_.Relax();
      yaw_actr_.Relax();
      roll_actr_.Relax();
      y_actr_.Relax();
      z_actr_.Relax();
      z_1_actr_.Relax();
      break;
    case CALI: {
      x_actr_.Control(setpoint_.x, dt_);
      pitch_actr_.Control(setpoint_.pitch, dt_);
      pitch_1_actr_.Control(setpoint_.pitch_1, dt_);
      if (roll_actr_.Ready()) {
        yaw_actr_.Control(setpoint_.yaw, dt_);
      } else {
        yaw_actr_.Relax();
      }
      if (pitch_actr_.Ready()) {
        if (!yaw_actr_.Ready()) {
          roll_actr_.Control(-M_PI * 0.4, dt_);
        } else {
          roll_actr_.Control(setpoint_.roll, dt_);
        }
      } else {
        roll_actr_.Relax();
      }
      y_actr_.Control(setpoint_.y, dt_);
      z_actr_.Control(setpoint_.z, dt_);
      z_1_actr_.Control(setpoint_.z_1, dt_);
      break;
    }
    case MOVE:
      ps.angle_ = {0.0, 0.0, 0.0};
      ps.pos_ = param_.zero_position;
      pitch_1_actr_.GroupControl(ps, pitch_actr_, pitch_1_actr_) >> yaw_actr_ >>
          roll_actr_ >> x_actr_ >> y_actr_ >> z_actr_ >> z_1_actr_;
      break;
  }

  ps.angle_ = {0.0, 0.0, 0.0};
  ps.pos_ = param_.zero_position;
  ps.target_angle_.pit = eulr_.pit - Component::Type::CycleValue(0.0f);
  ps.target_angle_.rol = eulr_.rol - Component::Type::CycleValue(0.0f);
  ps.target_angle_.yaw = -(eulr_.yaw - Component::Type::CycleValue(0.0f));
}

void OreCollect::UpdateFeedback() {
  x_actr_.UpdateFeedback();
  pitch_actr_.UpdateFeedback();
  pitch_1_actr_.UpdateFeedback();
  yaw_actr_.UpdateFeedback();
  roll_actr_.UpdateFeedback();
  y_actr_.UpdateFeedback();
  z_actr_.UpdateFeedback();
  z_1_actr_.UpdateFeedback();
}
