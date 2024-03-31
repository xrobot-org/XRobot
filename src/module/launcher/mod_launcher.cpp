#include "mod_launcher.hpp"

#include "bsp_pwm.h"
#include "bsp_time.h"

#define LAUNCHER_TRIG_SPEED_MAX (8191)

using namespace Module;

Launcher::Launcher(Param& param, float control_freq)
    : param_(param), ctrl_lock_(true) {
  for (size_t i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
    this->trig_actuator_.at(i) =
        new Component::PosActuator(param.trig_actr.at(i), control_freq);

    this->trig_motor_.at(i) =
        new Device::RMMotor(this->param_.trig_motor.at(i),
                            ("Launcher_Trig" + std::to_string(i)).c_str());
  }

  for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
    this->fric_actuator_.at(i) =
        new Component::SpeedActuator(param.fric_actr.at(i), control_freq);

    this->fric_motor_.at(i) =
        new Device::RMMotor(this->param_.fric_motor.at(i),
                            ("Launcher_Fric" + std::to_string(i)).c_str());
  }

  auto event_callback = [](LauncherEvent event, Launcher* launcher) {
    launcher->ctrl_lock_.Wait(UINT32_MAX);
    switch (event) { /* 根据event设置模式 */
      case CHANGE_FIRE_MODE_RELAX:
      case CHANGE_FIRE_MODE_SAFE:
      case CHANGE_FIRE_MODE_LOADED:
        launcher->SetFireMode(static_cast<FireMode>(event));
        break;
      case LAUNCHER_START_FIRE: /* 摩擦轮开启条件下，开火控制fire为ture */
        if (launcher->fire_ctrl_.fire_mode_ == LOADED) {
          launcher->fire_ctrl_.fire = true;
        }
        break;

      case CHANGE_TRIG_MODE_SINGLE:
        launcher->SetTrigMode(static_cast<TrigMode>(SINGLE));
        break;
      case CHANGE_TRIG_MODE_BURST:
        launcher->SetTrigMode(static_cast<TrigMode>(CONTINUED));
        break;
      case CHANGE_TRIG_MODE:
        launcher->SetTrigMode(static_cast<TrigMode>(
            (launcher->fire_ctrl_.trig_mode_ + 1) % CONTINUED));
        break;
      case OPEN_COVER:
        launcher->cover_mode_ = OPEN;
        break;
      case CLOSE_COVER:
        launcher->cover_mode_ = CLOSE;
        break;
      default:
        break;
    }

    launcher->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<Launcher*, LauncherEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  bsp_pwm_start(BSP_PWM_LAUNCHER_SERVO);
  bsp_pwm_set_comp(BSP_PWM_LAUNCHER_SERVO, this->param_.cover_close_duty);

  auto launcher_thread = [](Launcher* launcher) {
    auto ref_sub = Message::Subscriber<Device::Referee::Data>("referee");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      ref_sub.DumpData(launcher->raw_ref_);

      launcher->PraseRef();

      launcher->ctrl_lock_.Wait(UINT32_MAX);

      launcher->UpdateFeedback();
      launcher->Control();

      launcher->ctrl_lock_.Post();

      /* 运行结束，等待下一次唤醒 */
      launcher->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(launcher_thread, this, "launcher_thread",
                       MODULE_LAUNCHER_TASK_STACK_DEPTH,
                       System::Thread::MEDIUM);
  System::Timer::Create(this->DrawUIStatic, this, 2200);

  System::Timer::Create(this->DrawUIDynamic, this, 100);
}

void Launcher::UpdateFeedback() {
  const float LAST_TRIG_MOTOR_ANGLE = this->trig_motor_[0]->GetAngle();

  for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
    this->fric_motor_[i]->Update();
  }

  for (size_t i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
    this->trig_motor_[i]->Update();
  }

  const float DELTA_MOTOR_ANGLE =
      this->trig_motor_[0]->GetAngle() - LAST_TRIG_MOTOR_ANGLE;
  this->trig_angle_ += DELTA_MOTOR_ANGLE / this->param_.trig_gear_ratio;
}

void Launcher::Control() {
  this->now_ = bsp_time_get();
  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  this->HeatLimit();

  /* 根据开火模式计算发射行为 */
  uint32_t max_burst = 0;
  switch (this->fire_ctrl_.trig_mode_) {
    case SINGLE: /* 点射开火模式 */
      max_burst = 1;
      break;
    case BURST: /* 爆发开火模式 */
      max_burst = 5;
      break;
    default:
      max_burst = 1;
      break;
  }
  /* 发弹量设置 */
  switch (this->fire_ctrl_.trig_mode_) {
    case SINGLE: /* 点射开火模式 */
    case BURST:  /* 爆发开火模式 */

      /* 计算是否是第一次按下开火键 */
      this->fire_ctrl_.first_pressed_fire =
          this->fire_ctrl_.fire && !this->fire_ctrl_.last_fire;
      this->fire_ctrl_.last_fire = this->fire_ctrl_.fire;

      /* 设置要发射多少弹丸 */
      if (this->fire_ctrl_.first_pressed_fire && !this->fire_ctrl_.to_launch) {
        this->fire_ctrl_.to_launch =
            MIN(max_burst,
                (this->heat_ctrl_.available_shot - this->fire_ctrl_.launched));
      }

      /* 以下逻辑保证触发后一定会打完预设的弹丸，完成爆发 */
      if (this->fire_ctrl_.launched >= this->fire_ctrl_.to_launch) {
        this->fire_ctrl_.launch_delay = UINT32_MAX;
        this->fire_ctrl_.launched = 0;
        this->fire_ctrl_.to_launch = 0;
        this->fire_ctrl_.fire = false;
      } else {
        this->fire_ctrl_.launch_delay = this->param_.min_launch_delay;
      }
      break;

    case CONTINUED: { /* 持续开火模式 */
      float launch_freq = this->LimitLauncherFreq();
      this->fire_ctrl_.launch_delay =
          (launch_freq == 0.0f) ? UINT32_MAX
                                : static_cast<uint32_t>(1000.f / launch_freq);
      break;
    }
    default:
      break;
  }

  /* 根据模式选择是否使用计算出来的值 */
  switch (this->fire_ctrl_.fire_mode_) {
    case RELAX:
    case SAFE:
      this->fire_ctrl_.bullet_speed = 0.0f;
      this->fire_ctrl_.launch_delay = UINT32_MAX;
    case LOADED:
      break;
  }

  /* 计算摩擦轮转速的目标值 */
  this->setpoint_.fric_rpm_[1] = bullet_speed_to_fric_rpm(
      this->fire_ctrl_.bullet_speed, this->param_.fric_radius,
      (this->param_.model == LAUNCHER_MODEL_17MM));
  this->setpoint_.fric_rpm_[0] = -this->setpoint_.fric_rpm_[1];

  /* 计算拨弹电机位置的目标值 */
  if ((bsp_time_get_ms() - this->fire_ctrl_.last_launch) >=
      this->fire_ctrl_.launch_delay) {
    if ((fire_ctrl_.last_trig_angle - trig_angle_) / M_2PI *
            this->param_.num_trig_tooth >
        0.9) {
      if (!fire_ctrl_.stall) {
        fire_ctrl_.last_trig_angle = this->setpoint_.trig_angle_;
        /* 将拨弹电机角度进行循环加法，每次加(减)射出一颗弹丸的弧度变化 */
        this->setpoint_.trig_angle_ -= M_2PI / this->param_.num_trig_tooth;
      }
      /* 计算已发射弹丸 */
      this->fire_ctrl_.launched++;
      this->fire_ctrl_.last_launch = bsp_time_get_ms();
      fire_ctrl_.stall = false;
    } else if (param_.model == LAUNCHER_MODEL_42MM) {
      fire_ctrl_.stall = true;
      float tmp = this->setpoint_.trig_angle_;
      this->setpoint_.trig_angle_ = fire_ctrl_.last_trig_angle;
      fire_ctrl_.last_trig_angle = tmp;
      this->fire_ctrl_.last_launch = bsp_time_get_ms();
    }
  }
  /* 计算摩擦轮和拨弹盘并输出 */
  switch (this->fire_ctrl_.fire_mode_) {
    case RELAX:
      for (size_t i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
        this->trig_motor_[i]->Relax();
      }
      for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
        this->fric_motor_[i]->Relax();
      }
      bsp_pwm_stop(BSP_PWM_LAUNCHER_SERVO);
      break;

    case SAFE:
    case LOADED:
      for (int i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
        /* 控制拨弹电机 */
        float trig_out = this->trig_actuator_[i]->Calculate(
            this->setpoint_.trig_angle_,
            this->trig_motor_[i]->GetSpeed() / LAUNCHER_TRIG_SPEED_MAX,
            this->trig_angle_, this->dt_);

        this->trig_motor_[i]->Control(trig_out);
      }

      for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
        /* 控制摩擦轮 */
        float fric_out = this->fric_actuator_[i]->Calculate(
            this->setpoint_.fric_rpm_[i], this->fric_motor_[i]->GetSpeed(),
            this->dt_);

        this->fric_motor_[i]->Control(fric_out);
      }

      /* 根据弹仓盖开关状态更新弹舱盖打开时舵机PWM占空比 */
      if (this->cover_mode_ == OPEN) {
        bsp_pwm_start(BSP_PWM_LAUNCHER_SERVO);
        bsp_pwm_set_comp(BSP_PWM_LAUNCHER_SERVO, this->param_.cover_open_duty);
      } else {
        bsp_pwm_start(BSP_PWM_LAUNCHER_SERVO);
        bsp_pwm_set_comp(BSP_PWM_LAUNCHER_SERVO, this->param_.cover_close_duty);
      }
      break;
  }
}
/* 拨弹盘模式 */
/* SINGLE,BURST,CONTINUED,  */
void Launcher::SetTrigMode(TrigMode mode) {
  if (mode == this->fire_ctrl_.trig_mode_) {
    return;
  }

  this->fire_ctrl_.trig_mode_ = mode;
}
/* 设置摩擦轮模式 */
/* RELEX SAFE LOADED三种模式可以选择 */
void Launcher::SetFireMode(FireMode mode) {
  if (mode == this->fire_ctrl_.fire_mode_) { /* 未更改，return */
    return;
  }

  fire_ctrl_.fire = false;

  for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
    this->fric_actuator_[i]->Reset();
  } /* reset 所有电机执行器PID等参数 */

  if (mode == LOADED) {
    this->fire_ctrl_.to_launch = 0;
  }

  this->fire_ctrl_.fire_mode_ = mode;
}

void Launcher::HeatLimit() {
  if (this->ref_.status == Device::Referee::RUNNING) {
    /* 根据机器人型号获得对应数据 */
    if (this->param_.model == LAUNCHER_MODEL_42MM) {
      this->heat_ctrl_.heat = this->ref_.power_heat.launcher_42_heat;
      this->heat_ctrl_.heat_limit = this->ref_.robot_status.shooter_heat_limit;
      this->heat_ctrl_.speed_limit = BULLET_SPEED_LIMIT_42MM;
      this->heat_ctrl_.cooling_rate =
          this->ref_.robot_status.shooter_cooling_value;
      this->heat_ctrl_.heat_increase = GAME_HEAT_INCREASE_42MM;
    } else if (this->param_.model == LAUNCHER_MODEL_17MM) {
      this->heat_ctrl_.heat = this->ref_.power_heat.launcher_id1_17_heat;
      this->heat_ctrl_.heat_limit = this->ref_.robot_status.shooter_heat_limit;
      this->heat_ctrl_.speed_limit = BULLET_SPEED_LIMIT_17MM;
      this->heat_ctrl_.cooling_rate =
          this->ref_.robot_status.shooter_cooling_value;
      this->heat_ctrl_.heat_increase = GAME_HEAT_INCREASE_17MM;
    }
    /* 检测热量更新后,计算可发射弹丸 */

    if ((this->heat_ctrl_.heat != this->heat_ctrl_.last_heat) ||
        this->heat_ctrl_.available_shot == 0 || (this->heat_ctrl_.heat == 0)) {
      this->heat_ctrl_.available_shot = static_cast<uint32_t>(
          floorf((this->heat_ctrl_.heat_limit - this->heat_ctrl_.heat) /
                 this->heat_ctrl_.heat_increase));
      this->heat_ctrl_.last_heat = this->heat_ctrl_.heat;
    }
    this->fire_ctrl_.bullet_speed = this->heat_ctrl_.speed_limit;
  } else {
    /* 裁判系统离线，不启用热量控制 */
    this->heat_ctrl_.available_shot = 10;
    this->fire_ctrl_.bullet_speed = this->param_.default_bullet_speed;
  }
}

void Launcher::PraseRef() {
  memcpy(&(this->ref_.power_heat), &(this->raw_ref_.power_heat),
         sizeof(this->ref_.power_heat));
  memcpy(&(this->ref_.robot_status), &(this->raw_ref_.robot_status),
         sizeof(this->ref_.robot_status));
  memcpy(&(this->ref_.launcher_data), &(this->raw_ref_.launcher_data),
         sizeof(this->ref_.launcher_data));
  this->ref_.status = this->raw_ref_.status;
}

float Launcher::LimitLauncherFreq() { /* 热量限制计算 */
  float heat_percent = this->heat_ctrl_.heat / this->heat_ctrl_.heat_limit;
  float stable_freq =
      this->heat_ctrl_.cooling_rate / this->heat_ctrl_.heat_increase;
  if (this->param_.model == LAUNCHER_MODEL_42MM) {
    return stable_freq;
  } else {
    if (heat_percent > 0.9f) {
      return 0.5f;
    } else if (heat_percent > 0.8f) {
      return 1.0f;
    } else if (heat_percent > 0.6f) {
      return 2.0f * stable_freq;
    } else if (heat_percent > 0.2f) {
      return 3.0f * stable_freq;
    } else if (heat_percent > 0.1f) {
      return 4.0f * stable_freq;
    } else {
      return 5.0f;
    }
  }
}
void Launcher::DrawUIStatic(Launcher* launcher) {
  launcher->string_.Draw("SM", Component::UI::UI_GRAPHIC_OP_ADD,
                         Component::UI::UI_GRAPHIC_LAYER_CONST,
                         Component::UI::UI_GREEN, UI_DEFAULT_WIDTH * 10, 80,
                         UI_CHAR_DEFAULT_WIDTH,
                         static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                               REF_UI_RIGHT_START_W),
                         static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                               REF_UI_MODE_LINE3_H),
                         "SHOT  RELX  SAFE  LOAD");
  Device::Referee::AddUI(launcher->string_);

  launcher->string_.Draw("FM", Component::UI::UI_GRAPHIC_OP_ADD,
                         Component::UI::UI_GRAPHIC_LAYER_CONST,
                         Component::UI::UI_GREEN, UI_DEFAULT_WIDTH * 10, 80,
                         UI_CHAR_DEFAULT_WIDTH,
                         static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                               REF_UI_RIGHT_START_W),
                         static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                               REF_UI_MODE_LINE4_H),
                         "FIRE  SNGL  BRST  CONT");
  Device::Referee::AddUI(launcher->string_);

  float box_pos_left = 0.0f, box_pos_right = 0.0f;

  /* 更新发射器模式选择框 */
  switch (launcher->fire_ctrl_.fire_mode_) {
    case RELAX:
      box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
      break;
    case SAFE:
      box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
      break;
    case LOADED:
      box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
      break;
    default:
      box_pos_left = 0.0f;
      box_pos_right = 0.0f;
      break;
  }
  if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
    launcher->rectangle_.Draw(
        "FS", Component::UI::UI_GRAPHIC_OP_ADD,
        Component::UI::UI_GRAPHIC_LAYER_LAUNCHER, Component::UI::UI_GREEN,
        UI_DEFAULT_WIDTH,
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_left),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE3_H +
                              REF_UI_BOX_UP_OFFSET),
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_right),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE3_H +
                              REF_UI_BOX_BOT_OFFSET));
    Device::Referee::AddUI(launcher->rectangle_);
  }

  /* 更新开火模式选择框 */
  switch (launcher->fire_ctrl_.trig_mode_) {
    case SINGLE:
      box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
      break;
    case BURST:
      box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
      break;
    case CONTINUED:
      box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
      break;
    default:
      box_pos_left = 0.0f;
      box_pos_right = 0.0f;
      break;
  }
  if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
    launcher->rectangle_.Draw(
        "TS", Component::UI::UI_GRAPHIC_OP_ADD,
        Component::UI::UI_GRAPHIC_LAYER_LAUNCHER, Component::UI::UI_GREEN,
        UI_DEFAULT_WIDTH,
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_left),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE4_H +
                              REF_UI_BOX_UP_OFFSET),
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_right),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE4_H +
                              REF_UI_BOX_BOT_OFFSET));
    Device::Referee::AddUI(launcher->rectangle_);
  }

  launcher->arc_.Draw(
      "F0", Component::UI::UI_GRAPHIC_OP_ADD,
      Component::UI::UI_GRAPHIC_LAYER_LAUNCHER, Component::UI::UI_GREEN,
      static_cast<uint16_t>((launcher->fric_motor_[0]->GetSpeed() /
                             launcher->setpoint_.fric_rpm_[0]) *
                            180),
      360 - static_cast<uint16_t>((launcher->fric_motor_[0]->GetSpeed() /
                                   launcher->setpoint_.fric_rpm_[0]) *
                                  180),
      UI_DEFAULT_WIDTH * 5,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * REF_UI_RIGHT_FRIC),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                            REF_UI_MODE_LINE_BOTTOM_H),
      50, 50);
  Device::Referee::AddUI(launcher->arc_);

  uint16_t arc_start = static_cast<uint16_t>(
      -Component::Type::CycleValue(launcher->trig_angle_) / M_2PI * 360.f);
  uint16_t arc_end = static_cast<uint16_t>(
      -Component::Type::CycleValue(launcher->trig_angle_) / M_2PI * 360.f + 30);

  if (arc_end > 360) {
    arc_end = arc_end - 360;
  }

  launcher->arc_.Draw(
      "TP", Component::UI::UI_GRAPHIC_OP_ADD,
      Component::UI::UI_GRAPHIC_LAYER_LAUNCHER, Component::UI::UI_GREEN,
      arc_start, arc_end, UI_DEFAULT_WIDTH * 5,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * REF_UI_RIGHT_TRIC),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                            REF_UI_MODE_LINE_BOTTOM_H),
      40, 40);
  Device::Referee::AddUI(launcher->arc_);
}

void Launcher::DrawUIDynamic(Launcher* launcher) {
  float box_pos_left = 0.0f, box_pos_right = 0.0f;

  /* 更新发射器模式选择框 */
  switch (launcher->fire_ctrl_.fire_mode_) {
    case RELAX:
      box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
      break;
    case SAFE:
      box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
      break;
    case LOADED:
      box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
      break;
    default:
      box_pos_left = 0.0f;
      box_pos_right = 0.0f;
      break;
  }
  if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
    launcher->rectangle_.Draw(
        "FS", Component::UI::UI_GRAPHIC_OP_REWRITE,
        Component::UI::UI_GRAPHIC_LAYER_LAUNCHER, Component::UI::UI_GREEN,
        UI_DEFAULT_WIDTH,
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_left),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE3_H +
                              REF_UI_BOX_UP_OFFSET),
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_right),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE3_H +
                              REF_UI_BOX_BOT_OFFSET));
    Device::Referee::AddUI(launcher->rectangle_);
  }

  /* 更新开火模式选择框 */
  switch (launcher->fire_ctrl_.trig_mode_) {
    case SINGLE:
      box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
      break;
    case BURST:
      box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
      break;
    case CONTINUED:
      box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
      break;
    default:
      box_pos_left = 0.0f;
      box_pos_right = 0.0f;
      break;
  }

  if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
    launcher->rectangle_.Draw(
        "TS", Component::UI::UI_GRAPHIC_OP_REWRITE,
        Component::UI::UI_GRAPHIC_LAYER_LAUNCHER, Component::UI::UI_GREEN,
        UI_DEFAULT_WIDTH,
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_left),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE4_H +
                              REF_UI_BOX_UP_OFFSET),
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_right),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE4_H +
                              REF_UI_BOX_BOT_OFFSET));
    Device::Referee::AddUI(launcher->rectangle_);
  }

  uint16_t fric_1_sp =
      180 - static_cast<uint16_t>((launcher->fric_motor_[0]->GetSpeed() /
                                   launcher->setpoint_.fric_rpm_[0]) *
                                  180);
  uint16_t fric_2_sp =
      180 + static_cast<uint16_t>((launcher->fric_motor_[1]->GetSpeed() /
                                   launcher->setpoint_.fric_rpm_[1]) *
                                  180);

  Component::UI::Color fric_color = Component::UI::UI_GREEN;

  if (fric_1_sp > 360) {
    fric_1_sp = 0;
  }

  if (fric_2_sp > 360) {
    fric_2_sp = 360;
  }

  if (launcher->setpoint_.fric_rpm_[0] == 0) {
    fric_color = Component::UI::UI_GREEN;
    fric_1_sp = 270;
    fric_2_sp = 90;
  } else if ((fric_1_sp < 10) && (fric_2_sp > 350)) {
    fric_color = Component::UI::UI_GREEN;
  } else if ((fric_1_sp < 30) && (fric_2_sp > 330)) {
    fric_color = Component::UI::UI_ORANGE;
  } else {
    fric_1_sp = 270;
    fric_2_sp = 90;
    fric_color = Component::UI::UI_ORANGE;
  }

  launcher->arc_.Draw(
      "F0", Component::UI::UI_GRAPHIC_OP_REWRITE,
      Component::UI::UI_GRAPHIC_LAYER_LAUNCHER, fric_color, fric_1_sp,
      fric_2_sp, UI_DEFAULT_WIDTH * 5,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * REF_UI_RIGHT_FRIC),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                            REF_UI_MODE_LINE_BOTTOM_H),
      50, 50);
  Device::Referee::AddUI(launcher->arc_);

  uint16_t arc_start = static_cast<uint16_t>(
      -Component::Type::CycleValue(launcher->trig_angle_) / M_2PI * 360.f);
  uint16_t arc_end = static_cast<uint16_t>(
      -Component::Type::CycleValue(launcher->trig_angle_) / M_2PI * 360.f + 30);

  if (arc_end > 360) {
    arc_end = arc_end - 360;
  }

  launcher->arc_.Draw(
      "TP", Component::UI::UI_GRAPHIC_OP_REWRITE,
      Component::UI::UI_GRAPHIC_LAYER_LAUNCHER, Component::UI::UI_GREEN,
      arc_start, arc_end, UI_DEFAULT_WIDTH * 5,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * REF_UI_RIGHT_TRIC),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                            REF_UI_MODE_LINE_BOTTOM_H),
      40, 40);
  Device::Referee::AddUI(launcher->arc_);
}
