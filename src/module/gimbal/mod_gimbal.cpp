#include "mod_gimbal.hpp"

#include <comp_cmd.hpp>
#include <comp_type.hpp>
#include <comp_ui.hpp>

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
    gimbal->ctrl_lock_.Wait(UINT32_MAX);

    switch (event) {
      case SET_MODE_RELAX:
      case SET_MODE_ABSOLUTE:
        gimbal->SetMode(static_cast<Mode>(event));
        break;

      case START_AUTO_AIM:
        Component::CMD::SetCtrlSource(Component::CMD::CTRL_SOURCE_AI);
        break;
      case STOP_AUTO_AIM:
        Component::CMD::SetCtrlSource(Component::CMD::CTRL_SOURCE_RC);
        break;
      case SET_AUTOPATROL:
        gimbal->SetMode(static_cast<Mode>(AUTOPATROL));
        break;
    }
    gimbal->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<Gimbal*, GimbalEvent>(event_callback, this,
                                                      this->param_.EVENT_MAP);

  auto gimbal_thread = [](Gimbal* gimbal) {
    auto eulr_sub = Message::Subscriber<Component::Type::Eulr>("imu_eulr");

    auto gyro_sub = Message::Subscriber<Component::Type::Vector3>("imu_gyro");

    auto cmd_sub = Message::Subscriber<Component::CMD::GimbalCMD>("cmd_gimbal");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      /* 读取控制指令、姿态、IMU、电机反馈 */
      eulr_sub.DumpData(gimbal->eulr_); /* imu */
      gyro_sub.DumpData(gimbal->gyro_); /* imu */
      cmd_sub.DumpData(gimbal->cmd_);   /* cmd */

      gimbal->ctrl_lock_.Wait(UINT32_MAX);
      gimbal->UpdateFeedback();
      gimbal->Control();
      gimbal->ctrl_lock_.Post();

      gimbal->yaw_tp_.Publish(gimbal->yaw_);

      /* 运行结束，等待下一次唤醒 */
      gimbal->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(gimbal_thread, this, "gimbal_thread",
                       MODULE_GIMBAL_TASK_STACK_DEPTH, System::Thread::MEDIUM);

  System::Timer::Create(this->DrawUIStatic, this, 2000);

  System::Timer::Create(this->DrawUIDynamic, this, 60);
}

void Gimbal::UpdateFeedback() {
  this->pit_motor_.Update();
  this->yaw_motor_.Update();
  switch (this->mode_) {
    case RELAX:
    case ABSOLUTE:
      this->yaw_ = this->yaw_motor_.GetAngle() - this->param_.mech_zero.yaw;
      break;
    case AUTOPATROL:
      this->yaw_ = this->yaw_motor_.GetAngle() - this->param_.mech_zero.yaw -
                   this->param_.patrol_range *
                       sinf(this->param_.patrol_omega *
                            static_cast<float>(bsp_time_get_ms() -
                                               autopatrol_start_time_) /
                            1000.0f);

      break;
  }
}

void Gimbal::Control() {
  this->now_ = bsp_time_get();
  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

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

  /* 处理yaw控制命令，软件限位  */
  /* 某个轴max=min时不进行限位,配置文件默认不写 */
  if (param_.limit.yaw_max != param_.limit.yaw_min) {
    const float ENCODER_DELTA_MAX_YAW =
        this->param_.limit.yaw_max - this->yaw_motor_.GetAngle();
    const float ENCODER_DELTA_MIN_YAW =
        this->param_.limit.yaw_min - this->yaw_motor_.GetAngle();
    const float YAW_ERR = this->setpoint_.eulr_.yaw - eulr_.yaw;
    const float DELTA_MAX_YAW = ENCODER_DELTA_MAX_YAW - YAW_ERR;
    const float DELTA_MIN_YAW = ENCODER_DELTA_MIN_YAW - YAW_ERR;
    clampf(&(gimbal_yaw_cmd), DELTA_MIN_YAW, DELTA_MAX_YAW);
  }
  this->setpoint_.eulr_.yaw += gimbal_yaw_cmd;

  /* 处理pitch控制命令，软件限位 */
  if (param_.limit.pitch_max != param_.limit.pitch_min) {
    const float ENCODER_DELTA_MAX_PIT =
        this->param_.limit.pitch_max - this->pit_motor_.GetAngle();
    const float ENCODER_DELTA_MIN_PIT =
        this->param_.limit.pitch_min - this->pit_motor_.GetAngle();
    const float PIT_ERR = this->setpoint_.eulr_.pit - eulr_.pit;
    const float DELTA_MAX_PIT = ENCODER_DELTA_MAX_PIT - PIT_ERR;
    const float DELTA_MIN_PIT = ENCODER_DELTA_MIN_PIT - PIT_ERR;
    clampf(&(gimbal_pit_cmd), DELTA_MIN_PIT, DELTA_MAX_PIT);
  }
  this->setpoint_.eulr_.pit += gimbal_pit_cmd;

  /* 控制相关逻辑 */
  float yaw_out = 0;
  float pit_out = 0;
  float autopatrol_yaw = 0;
  switch (this->mode_) {
    case RELAX:
      this->yaw_motor_.Relax();
      this->pit_motor_.Relax();
      break;
    case ABSOLUTE:
      /* Yaw轴角速度环参数计算 */
      yaw_out = this->yaw_actuator_.Calculate(
          this->setpoint_.eulr_.yaw, this->gyro_.z, this->eulr_.yaw, this->dt_);

      pit_out = this->pit_actuator_.Calculate(
          this->setpoint_.eulr_.pit, this->gyro_.x, this->eulr_.pit, this->dt_);

      this->yaw_motor_.Control(yaw_out);
      this->pit_motor_.Control(pit_out);

      break;

    case AUTOPATROL:
      /* 以sin变化左右摆头 */
      autopatrol_yaw = this->setpoint_.eulr_.yaw +
                       this->param_.patrol_range *
                           sinf(this->param_.patrol_omega *
                                static_cast<float>(bsp_time_get_ms() -
                                                   autopatrol_start_time_) /
                                1000.0f);

      yaw_out = this->yaw_actuator_.Calculate(autopatrol_yaw, this->gyro_.z,
                                              this->eulr_.yaw, this->dt_);
      pit_out = this->pit_actuator_.Calculate(
          this->param_.patrol_hight, this->gyro_.x, this->eulr_.pit, this->dt_);

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

  if (mode == AUTOPATROL) {
    autopatrol_start_time_ = bsp_time_get_ms();
  }

  this->mode_ = mode;
}

void Gimbal::DrawUIStatic(Gimbal* gimbal) {
  gimbal->string_.Draw("GM", Component::UI::UI_GRAPHIC_OP_ADD,
                       Component::UI::UI_GRAPHIC_LAYER_CONST,
                       Component::UI::UI_GREEN, UI_DEFAULT_WIDTH * 10, 80,
                       UI_CHAR_DEFAULT_WIDTH,
                       static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                                             REF_UI_RIGHT_START_W),
                       static_cast<uint16_t>(Device::Referee::UIGetHeight() *
                                             REF_UI_MODE_LINE2_H),
                       "GMBL  RELX  ABSL  RLTV");
  Device::Referee::AddUI(gimbal->string_);

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
        "GS", Component::UI::UI_GRAPHIC_OP_ADD,
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
    Device::Referee::AddUI(gimbal->rectangle_);
  }
  gimbal->line_.Draw(
      "g", Component::UI::UI_GRAPHIC_OP_ADD,
      Component::UI::UI_GRAPHIC_LAYER_CONST, Component::UI::UI_GREEN,
      UI_DEFAULT_WIDTH * 3,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.4f),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2f),
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.4f),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2f + 50.f));
  Device::Referee::AddUI(gimbal->line_);
  gimbal->line_.Draw(
      "GA", Component::UI::UI_GRAPHIC_OP_ADD,
      Component::UI::UI_GRAPHIC_LAYER_GIMBAL, Component::UI::UI_GREEN,
      UI_DEFAULT_WIDTH * 12,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.4f),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2f),
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.4f +
                            -sinf(gimbal->yaw_) * 44),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2f +
                            cosf(gimbal->yaw_) * 44));
  Device::Referee::AddUI(gimbal->line_);
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
        "GS", Component::UI::UI_GRAPHIC_OP_REWRITE,
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
    Device::Referee::AddUI(gimbal->rectangle_);
  }
  gimbal->line_.Draw(
      "GA", Component::UI::UI_GRAPHIC_OP_REWRITE,
      Component::UI::UI_GRAPHIC_LAYER_GIMBAL, Component::UI::UI_GREEN,
      UI_DEFAULT_WIDTH * 12,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.4f),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2f),
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.4f +
                            -sinf(gimbal->yaw_) * 44),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2f +
                            cosf(gimbal->yaw_) * 44));
  Device::Referee::AddUI(gimbal->line_);
}
