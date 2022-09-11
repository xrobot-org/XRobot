/*
        DR16接收机

*/

#include "dev_dr16.hpp"

#include <string.h>

#include "bsp_uart.h"
#include "om.h"

#define DR16_CH_VALUE_MIN (364u)
#define DR16_CH_VALUE_MID (1024u)
#define DR16_CH_VALUE_MAX (1684u)

using namespace Device;

DR16::Data DR16::data_;

DR16::DR16()
    : new_(false), event_(System::Message::Event::FindEvent("cmd_event")) {
  auto rx_cplt_callback = [](void *arg) {
    DR16 *dr16 = static_cast<DR16 *>(arg);
    dr16->new_.GiveFromISR();
  };

  bsp_uart_register_callback(BSP_UART_DR16, BSP_UART_RX_CPLT_CB,
                             rx_cplt_callback, this);

  Component::CMD::RegisterController(this->cmd_);

  auto dr16_thread = [](void *arg) {
    DR16 *dr16 = static_cast<DR16 *>(arg);

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

  THREAD_DECLEAR(this->thread_, dr16_thread, 256, System::Thread::Realtime,
                 this);
}

bool DR16::StartRecv() {
  return bsp_uart_receive(BSP_UART_DR16, (uint8_t *)&(this->data_),
                          sizeof(this->data_), false) == BSP_OK;
}

bool DR16::DataCorrupted() {
  if ((this->data_.ch_r_x < DR16_CH_VALUE_MIN) ||
      (this->data_.ch_r_x > DR16_CH_VALUE_MAX))
    return true;

  if ((this->data_.ch_r_y < DR16_CH_VALUE_MIN) ||
      (this->data_.ch_r_y > DR16_CH_VALUE_MAX))
    return true;

  if ((this->data_.ch_l_x < DR16_CH_VALUE_MIN) ||
      (this->data_.ch_l_x > DR16_CH_VALUE_MAX))
    return true;

  if ((this->data_.ch_l_y < DR16_CH_VALUE_MIN) ||
      (this->data_.ch_l_y > DR16_CH_VALUE_MAX))
    return true;

  if (this->data_.sw_l == 0) return true;

  if (this->data_.sw_r == 0) return true;

  return false;
}

void DR16::PraseRC() {
  if (this->DataCorrupted()) {
    bsp_uart_abort_receive(BSP_UART_DR16);
    /* 等待错误包结束 */
    this->thread_.Sleep(3);

    return;
  }

  this->data_.key = 0;

  /* SwitchPos */
  if (this->data_.sw_l != this->last_data_.sw_l)
    this->event_.Active(SwitchPosLeftTop + this->data_.sw_l - 1);

  if (this->data_.sw_r != this->last_data_.sw_r)
    this->event_.Active(SwitchPosRightTop + this->data_.sw_r - 1);

  uint32_t tmp = 0;

  if (this->data_.key & (1 << (KeySHIFT - KeyW))) {
    tmp += KeyNum;
  }

  if (this->data_.key & (1 << (KeyCTRL - KeyW))) {
    tmp += 2 * KeyNum;
  }

  for (int i = 0; i < 16; i++) {
    if ((this->data_.key & (1 << i)) && !(this->last_data_.key & (1 << i))) {
      this->event_.Active(KeyW + i + tmp);
    }
  }

  if ((this->data_.key & ShiftCtrlWith(KeyE)) == ShiftCtrlWith(KeyE)) {
    this->ctrl_source_ = ControlSourceSW;
  }

  if ((this->data_.key & ShiftCtrlWith(KeyQ)) == ShiftCtrlWith(KeyQ)) {
    this->ctrl_source_ = ControlSourceMouse;
  }

  constexpr float full_range = (float)(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);

  if (this->ctrl_source_ == ControlSourceMouse) {
    if (this->data_.press_l && !this->last_data_.press_l)
      this->event_.Active(KeyLClick);
    if (this->data_.press_r && !this->last_data_.press_r)
      this->event_.Active(KeyRClick);

    /* Chassis Control */
    if (this->data_.key & KeyA) this->cmd_.data_.chassis.x -= 0.5;

    if (this->data_.key & KeyD) this->cmd_.data_.chassis.x += 0.5;

    if (this->data_.key & KeyS) this->cmd_.data_.chassis.y -= 0.5;

    if (this->data_.key & KeyW) this->cmd_.data_.chassis.y += 0.5;

    if (this->data_.key & KeySHIFT) {
      this->cmd_.data_.chassis.x *= 2;
      this->cmd_.data_.chassis.y *= 2;
    }

    this->cmd_.data_.chassis.z = 0.0f;

    /* Gimbal Control */
    this->cmd_.data_.gimbal.eulr.pit = this->data_.y / 32768.0f;
    this->cmd_.data_.gimbal.eulr.yaw = this->data_.x / 32768.0f;
    this->cmd_.data_.gimbal.eulr.rol = 0.0f;

  } else if (this->ctrl_source_ == ControlSourceSW) {
    /* Chassis Control */
    this->cmd_.data_.chassis.x =
        2 * ((float)this->data_.ch_l_x - DR16_CH_VALUE_MID) / full_range;
    this->cmd_.data_.chassis.y =
        2 * ((float)this->data_.ch_l_y - DR16_CH_VALUE_MID) / full_range;
    this->cmd_.data_.chassis.z = 0.0f;

    /* Gimbal Control */
    this->cmd_.data_.gimbal.eulr.yaw =
        2 * ((float)this->data_.ch_r_x - DR16_CH_VALUE_MID) / full_range;
    this->cmd_.data_.gimbal.eulr.pit =
        2 * ((float)this->data_.ch_r_y - DR16_CH_VALUE_MID) / full_range;
    this->cmd_.data_.gimbal.eulr.rol = 0.0f;
  }

  this->cmd_.data_.online = true;

  this->cmd_.data_.ctrl_source = Component::CMD::ControlSourceRC;

  this->cmd_.Publish();

  memcpy(&(this->last_data_), &(this->data_), sizeof(Data));
}

void DR16::Offline() {
  this->cmd_.data_.online = false;

  this->ctrl_source_ = ControlSourceSW;

  memset(&(this->cmd_.data_), 0, sizeof(this->cmd_.data_));

  memset(&(this->last_data_), 0, sizeof(this->last_data_));

  this->cmd_.Publish();
}
