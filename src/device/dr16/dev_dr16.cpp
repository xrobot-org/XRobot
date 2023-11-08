/*
        DR16接收机

*/

#include "dev_dr16.hpp"

#include "bsp_uart.h"
#include "dev_referee.hpp"

#define DR16_CH_VALUE_MIN (364u)
#define DR16_CH_VALUE_MID (1024u)

#define DR16_CH_VALUE_MAX (1684u)

using namespace Device;

DR16::Data DR16::data_;

DR16::DR16()
    : event_(Message::Event::FindEvent("cmd_event")), cmd_tp_("cmd_rc") {
  auto rx_cplt_callback = [](void *arg) {
    DR16 *dr16 = static_cast<DR16 *>(arg);
    System::Signal::Action(dr16->thread_, 0);
  };

  bsp_uart_register_callback(BSP_UART_DR16, BSP_UART_RX_CPLT_CB,
                             rx_cplt_callback, this);

  Component::CMD::RegisterController(this->cmd_tp_);

  auto dr16_thread = [](DR16 *dr16) {
    while (1) {
      /* 开启DMA */
      dr16->StartRecv();

      /* 等待DMA完成 */
      if (System::Signal::Wait(0, 20)) {
        /* 进行解析 */
        dr16->PraseRC();
      } else {
        /* 处理遥控器离线 */
        dr16->Offline();
      }
    }
  };

  this->thread_.Create(dr16_thread, this, "dr16_thread",
                       DEVICE_DR16_TASK_STACK_DEPTH, System::Thread::REALTIME);
  System::Timer::Create(this->DrawUIStatic, this, 2400);

  System::Timer::Create(this->DrawUIDynamic, this, 200);
}

bool DR16::StartRecv() {
  return bsp_uart_receive(BSP_UART_DR16,
                          reinterpret_cast<uint8_t *>(&this->data_),
                          sizeof(this->data_), false) == BSP_OK;
}

bool DR16::DataCorrupted() {
  if ((this->data_.ch_r_x < DR16_CH_VALUE_MIN) ||
      (this->data_.ch_r_x > DR16_CH_VALUE_MAX)) {
    return true;
  }
  if ((this->data_.ch_r_y < DR16_CH_VALUE_MIN) ||
      (this->data_.ch_r_y > DR16_CH_VALUE_MAX)) {
    return true;
  }
  if ((this->data_.ch_l_x < DR16_CH_VALUE_MIN) ||
      (this->data_.ch_l_x > DR16_CH_VALUE_MAX)) {
    return true;
  }
  if ((this->data_.ch_l_y < DR16_CH_VALUE_MIN) ||
      (this->data_.ch_l_y > DR16_CH_VALUE_MAX)) {
    return true;
  }
  if (this->data_.sw_l == 0) {
    return true;
  }

  if (this->data_.sw_r == 0) {
    return true;
  }

  return false;
}

void DR16::PraseRC() {
  if (this->DataCorrupted()) {
    bsp_uart_abort_receive(BSP_UART_DR16);
    /* 等待错误包结束 */
    this->thread_.Sleep(3);

    return;
  }

  /* 检测拨杆开关 */
  if (this->data_.sw_l != this->last_data_.sw_l) {
    this->event_.Active(DR16_SW_L_POS_TOP + this->data_.sw_l - 1);
  }

  if (this->data_.sw_r != this->last_data_.sw_r) {
    this->event_.Active(DR16_SW_R_POS_TOP + this->data_.sw_r - 1);
  }

  uint32_t tmp = 0;

  /* 检测Shift */
  if (this->data_.key & RawValue(KEY_SHIFT)) {
    tmp += KEY_NUM;
  }

  /* 检测Ctrl */
  if (this->data_.key & RawValue(KEY_CTRL)) {
    tmp += 2 * KEY_NUM;
  }

  /* 检测剩余按键 */
  for (int i = 0; i < 16; i++) {
    if ((this->data_.key & (1 << i)) && !(this->last_data_.key & (1 << i))) {
      this->event_.Active(KEY_W + i + tmp);
    }
  }

  /* 控制权切换 */
  if (((this->data_.key &
        (RawValue(KEY_SHIFT) | RawValue(KEY_CTRL) | RawValue(KEY_Q))) ==
       (RawValue(KEY_SHIFT) | RawValue(KEY_CTRL) | RawValue(KEY_Q)))) {
    this->ctrl_source_ = DR16_CTRL_SOURCE_SW;
  }

  if (((this->data_.key &
        (RawValue(KEY_SHIFT) | RawValue(KEY_CTRL) | RawValue(KEY_E))) ==
       (RawValue(KEY_SHIFT) | RawValue(KEY_CTRL) | RawValue(KEY_E)))) {
    this->ctrl_source_ = DR16_CTRL_SOURCE_MOUSE;
  }

  /* 最大量程 */
  constexpr float FULL_RANGE =
      static_cast<float>(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);

  memset(&this->cmd_, 0, sizeof(this->cmd_));

  if (this->ctrl_source_ == DR16_CTRL_SOURCE_MOUSE) { /* 键鼠控制 */
    /* 鼠标左右键 */
    if (this->data_.press_l && !this->last_data_.press_l) {
      this->event_.Active(KEY_L_PRESS);
    }
    if (this->data_.press_r && !this->last_data_.press_r) {
      this->event_.Active(KEY_R_PRESS);
    }
    if (!this->data_.press_l && this->last_data_.press_l) {
      this->event_.Active(KEY_L_RELEASE);
    }
    if (!this->data_.press_r && this->last_data_.press_r) {
      this->event_.Active(KEY_R_RELEASE);
    }
    /* 底盘控制 */
    if (this->data_.key & RawValue(KEY_A)) {
      this->cmd_.chassis.x -= 0.5;
    }

    if (this->data_.key & RawValue(KEY_D)) {
      this->cmd_.chassis.x += 0.5;
    }

    if (this->data_.key & RawValue(KEY_S)) {
      this->cmd_.chassis.y -= 0.5;
    }

    if (this->data_.key & RawValue(KEY_W)) {
      this->cmd_.chassis.y += 0.5;
    }

    /* 加速 */
    if (this->data_.key & RawValue(KEY_SHIFT)) {
      this->cmd_.chassis.x *= 2;
      this->cmd_.chassis.y *= 2;
    }

    this->cmd_.chassis.z = 0.0f;

    /* 云台控制 */
    this->cmd_.gimbal.eulr.pit =
        -static_cast<float>(this->data_.y) / 32768.0f * 1000.0f;
    this->cmd_.gimbal.eulr.yaw =
        -static_cast<float>(this->data_.x) / 32768.0f * 1000.0f;
    this->cmd_.gimbal.eulr.rol = 0.0f;

  } else if (this->ctrl_source_ == DR16_CTRL_SOURCE_SW) { /* 遥控器控制 */
    /* Chassis Control */
    this->cmd_.chassis.x =
        2 * (static_cast<float>(this->data_.ch_l_x) - DR16_CH_VALUE_MID) /
        FULL_RANGE;
    this->cmd_.chassis.y =
        2 * (static_cast<float>(this->data_.ch_l_y) - DR16_CH_VALUE_MID) /
        FULL_RANGE;
    this->cmd_.chassis.z =
        -2 * (static_cast<float>(this->data_.ch_r_x) - DR16_CH_VALUE_MID) /
        FULL_RANGE;

    /* Gimbal Control */
    this->cmd_.gimbal.eulr.yaw =
        -2 * (static_cast<float>(this->data_.ch_r_x) - DR16_CH_VALUE_MID) /
        FULL_RANGE;
    this->cmd_.gimbal.eulr.pit =
        2 * (static_cast<float>(this->data_.ch_r_y) - DR16_CH_VALUE_MID) /
        FULL_RANGE;
    this->cmd_.gimbal.eulr.rol = 0.0f;
  }

  this->cmd_.gimbal.mode = Component::CMD::GIMBAL_RELATIVE_CTRL;

  this->cmd_.online = true;

  this->cmd_.ctrl_source = Component::CMD::CTRL_SOURCE_RC;

  this->cmd_tp_.Publish(this->cmd_);

  memcpy(&(this->last_data_), &(this->data_), sizeof(Data));
}

void DR16::Offline() {
  this->cmd_.online = false;

  this->ctrl_source_ = DR16_CTRL_SOURCE_SW;

  memset(&(this->cmd_), 0, sizeof(this->cmd_));

  memset(&(this->last_data_), 0, sizeof(this->last_data_));

  this->cmd_tp_.Publish(this->cmd_);
}

void DR16::DrawUIStatic(DR16 *dr16) {
  dr16->string_.Draw(
      "DM", Component::UI::UI_GRAPHIC_OP_ADD,
      Component::UI::UI_GRAPHIC_LAYER_CONST, Component::UI::UI_GREEN,
      UI_DEFAULT_WIDTH * 10, 80, UI_CHAR_DEFAULT_WIDTH,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() *
                            REF_UI_RIGHT_START_W),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.4f),
      "CTRL  JS  KM");

  switch (dr16->ctrl_source_) {
    case DR16_CTRL_SOURCE_MOUSE:
      dr16->rectangle_.Draw(
          "DS", Component::UI::UI_GRAPHIC_OP_ADD,
          Component::UI::UI_GRAPHIC_LAYER_CMD, Component::UI::UI_GREEN,
          UI_DEFAULT_WIDTH,
          static_cast<uint16_t>(
              Device::Referee::UIGetWidth() * REF_UI_RIGHT_START_W + 96.f),
          static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.4f +
                                REF_UI_BOX_UP_OFFSET),
          static_cast<uint16_t>(
              Device::Referee::UIGetWidth() * REF_UI_RIGHT_START_W + 120.f),
          static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.4f +
                                REF_UI_BOX_BOT_OFFSET));
      Device::Referee::AddUI(dr16->rectangle_);
      break;
    case DR16_CTRL_SOURCE_SW:
      dr16->rectangle_.Draw(
          "DS", Component::UI::UI_GRAPHIC_OP_ADD,
          Component::UI::UI_GRAPHIC_LAYER_CMD, Component::UI::UI_GREEN,
          UI_DEFAULT_WIDTH,
          static_cast<uint16_t>(
              Device::Referee::UIGetWidth() * REF_UI_RIGHT_START_W + 56.f),
          static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.4f +
                                REF_UI_BOX_UP_OFFSET),
          static_cast<uint16_t>(
              Device::Referee::UIGetWidth() * REF_UI_RIGHT_START_W + 80.f),
          static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.4f +
                                REF_UI_BOX_BOT_OFFSET));
      Device::Referee::AddUI(dr16->rectangle_);
      break;
  }
  Device::Referee::AddUI(dr16->string_);
}

void DR16::DrawUIDynamic(DR16 *dr16) {
  switch (dr16->ctrl_source_) {
    case DR16_CTRL_SOURCE_MOUSE:
      dr16->rectangle_.Draw(
          "DS", Component::UI::UI_GRAPHIC_OP_REWRITE,
          Component::UI::UI_GRAPHIC_LAYER_CMD, Component::UI::UI_GREEN,
          UI_DEFAULT_WIDTH,
          static_cast<uint16_t>(
              Device::Referee::UIGetWidth() * REF_UI_RIGHT_START_W + 96.f),
          static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.4f +
                                REF_UI_BOX_UP_OFFSET),
          static_cast<uint16_t>(
              Device::Referee::UIGetWidth() * REF_UI_RIGHT_START_W + 120.f),
          static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.4f +
                                REF_UI_BOX_BOT_OFFSET));
      Device::Referee::AddUI(dr16->rectangle_);
      break;
    case DR16_CTRL_SOURCE_SW:
      dr16->rectangle_.Draw(
          "DS", Component::UI::UI_GRAPHIC_OP_REWRITE,
          Component::UI::UI_GRAPHIC_LAYER_CMD, Component::UI::UI_ORANGE,
          UI_DEFAULT_WIDTH,
          static_cast<uint16_t>(
              Device::Referee::UIGetWidth() * REF_UI_RIGHT_START_W + 56.f),
          static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.4f +
                                REF_UI_BOX_UP_OFFSET),
          static_cast<uint16_t>(
              Device::Referee::UIGetWidth() * REF_UI_RIGHT_START_W + 80.f),
          static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.4f +
                                REF_UI_BOX_BOT_OFFSET));
      Device::Referee::AddUI(dr16->rectangle_);
      break;
  }
}
