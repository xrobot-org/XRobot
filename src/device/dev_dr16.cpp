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

DR16::DR16() {
  auto rx_cplt_callback = [](void *arg) {
    DR16 *dr16 = (DR16 *)arg;
    dr16->new_.GiveFromISR();
  };

  bsp_uart_register_callback(BSP_UART_DR16, BSP_UART_RX_CPLT_CB,
                             rx_cplt_callback, this);

  auto dr16_thread = [](void *arg) {
    DR16 *dr16 = (DR16 *)arg;

    DECLARE_TOPIC(rc_tp, dr16->rc_, "cmd_rc", true);

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

      rc_tp.Publish();
    }
  };

  THREAD_DECLEAR(this->thread_, dr16_thread, 128, System::Thread::Realtime,
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

  float full_range = (float)(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);

  this->rc_.ch.r.x =
      2 * ((float)this->data_.ch_r_x - DR16_CH_VALUE_MID) / full_range;
  this->rc_.ch.r.y =
      2 * ((float)this->data_.ch_r_y - DR16_CH_VALUE_MID) / full_range;
  this->rc_.ch.l.x =
      2 * ((float)this->data_.ch_l_x - DR16_CH_VALUE_MID) / full_range;
  this->rc_.ch.l.y =
      2 * ((float)this->data_.ch_l_y - DR16_CH_VALUE_MID) / full_range;

  this->rc_.sw_l = (Component::CMD::SwitchPos)this->data_.sw_l;
  this->rc_.sw_r = (Component::CMD::SwitchPos)this->data_.sw_r;

  this->rc_.mouse.x = this->data_.x;
  this->rc_.mouse.y = this->data_.y;
  this->rc_.mouse.z = this->data_.z;

  this->rc_.mouse.click.l = this->data_.press_l;
  this->rc_.mouse.click.r = this->data_.press_r;

  this->rc_.key = this->data_.key;

  this->rc_.ch_res = ((float)this->data_.res - DR16_CH_VALUE_MID) / full_range;
}

void DR16::Offline() { memset(&(this->rc_), 0, sizeof(this->rc_)); }
