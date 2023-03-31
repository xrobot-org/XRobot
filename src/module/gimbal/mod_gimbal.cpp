/*
 * 云台模组
 */

#include "mod_gimbal.hpp"

#include <comp_type.hpp>

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
  auto event_callback = [](GimbalEvent event, Gimbal* gimbal) {
    gimbal->ctrl_lock_.Take(UINT32_MAX);

    gimbal->SetMode(static_cast<Mode>(event));

    gimbal->ctrl_lock_.Give();
  };

  Component::CMD::RegisterEvent<Gimbal*, GimbalEvent>(event_callback, this,
                                                      this->param_.EVENT_MAP);

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

  this->thread_.Create(gimbal_thread, this, "gimbal_thread",
                       MODULE_GIMBAL_TASK_STACK_DEPTH, System::Thread::MEDIUM);

  System::Timer::Create(this->DrawUIStatic, this, 2100);

  System::Timer::Create(this->DrawUIDynamic, this, 200);
}

void Gimbal::UpdateFeedback() {
  this->pit_motor_.Update();
  this->yaw_motor_.Update();

  this->yaw_ = this->yaw_motor_.GetAngle() - this->param_.mech_zero.yaw;
}

void Gimbal::Control() {
  this->now_ = bsp_time_get();
  this->dt_ = this->now_ - this->last_wakeup_;
  this->last_wakeup_ = this->now_;

  float gimbal_pit_cmd = 0.0f;
  float gimbal_yaw_cmd = 0.0f;

  /* yaw坐标正方向与遥控器操作逻辑相反 */
  if (this->cmd_.mode == Component::CMD::GIMBAL_RELATIVE_CTRL) {
    gimbal_yaw_cmd = this->cmd_.eulr.yaw * this->dt_ * GIMBAL_MAX_SPEED;
    gimbal_pit_cmd = this->cmd_.eulr.pit * this->dt_ * GIMBAL_MAX_SPEED;

  } else {
    gimbal_yaw_cmd = Component::Type::CycleValue(this->cmd_.eulr.yaw) -
                     this->setpoint_.eulr_.yaw;
    gimbal_pit_cmd = Component::Type::CycleValue(this->cmd_.eulr.pit) -
                     this->setpoint_.eulr_.pit;
  }

  /* 处理yaw控制命令 */
  this->setpoint_.eulr_.yaw += gimbal_yaw_cmd;

  /* 处理pitch控制命令，软件限位 */
  const float ENCODER_DELTA_MAX =
      this->param_.limit.pitch_max - this->pit_motor_.GetAngle();
  const float ENCODER_DELTA_MIN =
      this->param_.limit.pitch_min - this->pit_motor_.GetAngle();
  const float PIT_ERR = this->setpoint_.eulr_.pit - eulr_.pit;
  const float DELTA_MAX = ENCODER_DELTA_MAX - PIT_ERR;
  const float DELTA_MIN = ENCODER_DELTA_MIN - PIT_ERR;
  clampf(&(gimbal_pit_cmd), DELTA_MIN, DELTA_MAX);
  this->setpoint_.eulr_.pit += gimbal_pit_cmd;

  /* 控制相关逻辑 */
  switch (this->mode_) {
    case RELAX:
      this->yaw_motor_.Relax();
      this->pit_motor_.Relax();
      break;
    case ABSOLUTE:
      /* Yaw轴角速度环参数计算 */
      float yaw_out = this->yaw_actuator_.Calculate(
          this->setpoint_.eulr_.yaw, this->gyro_.z, this->eulr_.yaw, this->dt_);

      float pit_out = this->pit_actuator_.Calculate(
          this->setpoint_.eulr_.pit, this->gyro_.x, this->eulr_.pit, this->dt_);

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

  memcpy(&(this->setpoint_.eulr_), &(this->eulr_),
         sizeof(this->setpoint_.eulr_)); /* 切换模式后重置设定值 */
  if (this->mode_ == RELAX) {
    if (mode == ABSOLUTE) {
      this->setpoint_.eulr_.yaw = this->eulr_.yaw;
    }
  }

  this->mode_ = mode;

  memcpy(&(this->setpoint_.eulr_), &(this->eulr_),
         sizeof(this->setpoint_.eulr_)); /* 切换模式后重置设定值 */
  if (this->mode_ == RELAX) {
    if (mode == ABSOLUTE) {
      this->setpoint_.eulr_.yaw = this->eulr_.yaw;
    }
  }

  this->mode_ = mode;
}

void Gimbal::DrawUIStatic(Gimbal* gimbal) {
  gimbal->string_.Draw(
      &(gimbal->ui_string_data_), "GM", Component::UI::UI_GRAPHIC_OP_ADD,
      Component::UI::UI_GRAPHIC_LAYER_CONST, Component::UI::UI_GREEN,
      UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                            REF_UI_RIGHT_START_W),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                            REF_UI_MODE_LINE2_H),
      "GMBL  RELX  ABSL  RLTV");
  float box_pos_left = 0.0f, box_pos_right = 0.0f;

  /* 更新云台模式选择框 */
  switch (gimbal->mode_) {
    case RELAX:
      box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
      break;
    case ABSOLUTE:
      if (gimbal->cmd_.mode == Component::CMD::GIMBAL_ABSOLUTE_CTRL) {
        box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
        box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
      } else {
        box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
        box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
      }
      break;
    default:
      box_pos_left = 0.0f;
      box_pos_right = 0.0f;
      break;
  }
  if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
    gimbal->rectangle_.Draw(
        &(gimbal->ui_mode_data_), "GS", Component::UI::UI_GRAPHIC_OP_ADD,
        Component::UI::UI_GRAPHIC_LAYER_GIMBAL, Component::UI::UI_GREEN,
        UI_DEFAULT_WIDTH,
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_left),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE2_H +
                              REF_UI_BOX_UP_OFFSET),
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_right),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE2_H +
                              REF_UI_BOX_BOT_OFFSET));
  }

  Device::Referee::AddUI(gimbal->ui_string_data_);
  Device::Referee::AddUI(gimbal->ui_mode_data_);
}

void Gimbal::DrawUIDynamic(Gimbal* gimbal) {
  float box_pos_left = 0.0f, box_pos_right = 0.0f;

  /* 更新云台模式选择框 */
  switch (gimbal->mode_) {
    case RELAX:
      box_pos_left = REF_UI_MODE_OFFSET_2_LEFT;
      box_pos_right = REF_UI_MODE_OFFSET_2_RIGHT;
      break;
    case ABSOLUTE:
      if (gimbal->cmd_.mode == Component::CMD::GIMBAL_ABSOLUTE_CTRL) {
        box_pos_left = REF_UI_MODE_OFFSET_3_LEFT;
        box_pos_right = REF_UI_MODE_OFFSET_3_RIGHT;
      } else {
        box_pos_left = REF_UI_MODE_OFFSET_4_LEFT;
        box_pos_right = REF_UI_MODE_OFFSET_4_RIGHT;
      }
      break;
    default:
      box_pos_left = 0.0f;
      box_pos_right = 0.0f;
      break;
  }
  if (box_pos_left != 0.0f && box_pos_right != 0.0f) {
    gimbal->rectangle_.Draw(
        &(gimbal->ui_mode_data_), "GS", Component::UI::UI_GRAPHIC_OP_REWRITE,
        Component::UI::UI_GRAPHIC_LAYER_GIMBAL, Component::UI::UI_GREEN,
        UI_DEFAULT_WIDTH,
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_left),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE2_H +
                              REF_UI_BOX_UP_OFFSET),
        static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                  REF_UI_RIGHT_START_W +
                              box_pos_right),
        static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                  REF_UI_MODE_LINE2_H +
                              REF_UI_BOX_BOT_OFFSET));
  }

  Device::Referee::AddUI(gimbal->ui_mode_data_);
}
