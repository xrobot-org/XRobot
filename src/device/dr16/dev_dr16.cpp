/*
        DR16接收机

*/

#include "dev_dr16.hpp"

#include "bsp_uart.h"

#define DR16_CH_VALUE_MIN (364u)
#define DR16_CH_VALUE_MID (1024u)

#define DR16_CH_VALUE_MAX (1684u)

using namespace Device;

DR16::Data DR16::data_;

DR16::DR16()
    : new_(false),
      event_(Message::Event::FindEvent("cmd_event")),
      cmd_tp_("cmd_rc") {
  auto rx_cplt_callback = [](void *arg) {
    DR16 *dr16 = static_cast<DR16 *>(arg);
    dr16->new_.GiveFromISR();
  };

  bsp_uart_register_callback(BSP_UART_DR16, BSP_UART_RX_CPLT_CB,
                             rx_cplt_callback, this);

  Component::CMD::RegisterController(this->cmd_tp_);

  auto dr16_thread = [](DR16 *dr16) {
    while (1) {
      /* 开启DMA */
      dr16->StartRecv();

      /* 等待DMA完成 */
      if (dr16->new_.Take(20)) {
        /* 进行解析 */
        dr16->PraseRC();
      } else {
        /* 处理遥控器离线 */
        dr16->Offline();
      }
    }
  };

  this->thread_.Create(dr16_thread, this, "dr16_thread", 256,
                       System::Thread::REALTIME);
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
    this->thread_.SleepUntil(3);

    return;
  }

  this->data_.key = 0;

  /* 检测拨杆开关 */
  if (this->data_.sw_l != this->last_data_.sw_l) {
    this->event_.Active(DR16_SW_L_POS_TOP + this->data_.sw_l - 1);
  }

  if (this->data_.sw_r != this->last_data_.sw_r) {
    this->event_.Active(DR16_SW_R_POS_TOP + this->data_.sw_r - 1);
  }

  uint32_t tmp = 0;

  /* 检测Shift */
  if (this->data_.key & (1 << (KEY_SHIFT - KEY_W))) {
    tmp += KEY_NUM;
  }

  /* 检测Ctrl */
  if (this->data_.key & (1 << (KEY_CTRL - KEY_W))) {
    tmp += 2 * KEY_NUM;
  }

  /* 检测剩余按键 */
  for (int i = 0; i < 16; i++) {
    if ((this->data_.key & (1 << i)) && !(this->last_data_.key & (1 << i))) {
      this->event_.Active(KEY_W + i + tmp);
    }
  }

  /* 控制权切换 */
  if ((this->data_.key & ShiftCtrlWith(KEY_E)) == ShiftCtrlWith(KEY_E)) {
    this->ctrl_source_ = DR16_CTRL_SOURCE_SW;
  }

  if ((this->data_.key & ShiftCtrlWith(KEY_Q)) == ShiftCtrlWith(KEY_Q)) {
    this->ctrl_source_ = DR16_CTRL_SOURCE_MOUSE;
  }

  /* 最大量程 */
  constexpr float FULL_RANGE =
      static_cast<float>(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);

  if (this->ctrl_source_ == DR16_CTRL_SOURCE_MOUSE) { /* 键鼠控制 */
    /* 鼠标左右键 */
    if (this->data_.press_l && !this->last_data_.press_l) {
      this->event_.Active(KEY_L_CLICK);
    }
    if (this->data_.press_r && !this->last_data_.press_r) {
      this->event_.Active(KEY_R_CLICK);
    }
    /* 底盘控制 */
    if (this->data_.key & KEY_A) {
      this->cmd_.chassis.x -= 0.5;
    }

    if (this->data_.key & KEY_D) {
      this->cmd_.chassis.x += 0.5;
    }

    if (this->data_.key & KEY_S) {
      this->cmd_.chassis.y -= 0.5;
    }

    if (this->data_.key & KEY_W) {
      this->cmd_.chassis.y += 0.5;
    }

    /* 加速 */
    if (this->data_.key & KEY_SHIFT) {
      this->cmd_.chassis.x *= 2;
      this->cmd_.chassis.y *= 2;
    }

    this->cmd_.chassis.z = 0.0f;

    /* 云台控制 */
    this->cmd_.gimbal.eulr.pit = static_cast<float>(this->data_.y) / 32768.0f;
    this->cmd_.gimbal.eulr.yaw = static_cast<float>(this->data_.x) / 32768.0f;
    this->cmd_.gimbal.eulr.rol = 0.0f;

  } else if (this->ctrl_source_ == DR16_CTRL_SOURCE_SW) { /* 遥控器控制 */
    /* Chassis Control */
    this->cmd_.chassis.x =
        2 * (static_cast<float>(this->data_.ch_l_x) - DR16_CH_VALUE_MID) /
        FULL_RANGE;
    this->cmd_.chassis.y =
        2 * (static_cast<float>(this->data_.ch_l_y) - DR16_CH_VALUE_MID) /
        FULL_RANGE;
    this->cmd_.chassis.z = 0.0f;

    /* Gimbal Control */
    this->cmd_.gimbal.eulr.yaw =
        2 * (static_cast<float>(this->data_.ch_r_x) - DR16_CH_VALUE_MID) /
        FULL_RANGE;
    this->cmd_.gimbal.eulr.pit =
        2 * (static_cast<float>(this->data_.ch_r_y) - DR16_CH_VALUE_MID) /
        FULL_RANGE;
    this->cmd_.gimbal.eulr.rol = 0.0f;
  }

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
