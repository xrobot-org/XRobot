#include "dev_cap.hpp"

#include <stdbool.h>
#include <string.h>

#include "comp_utils.hpp"
#include "om.h"

#define CAP_RES (100) /* 电容数据分辨率 */

#define CAP_CUTOFF_VOLT \
  13.0f /* 电容截止电压，未接升压模块时要高于电调最低工作电压 */

using namespace Device;

Cap::Cap(Cap::Param &param) : param_(param) {
  auto rx_callback = [](om_msg_t *msg, void *arg) {
    Cap *cap = (Cap *)arg;
    can_rx_item_t *rx = (can_rx_item_t *)msg->buff;

    rx->index -= cap->param_.index;

    if (rx->index == 0) {
      cap->control_feedback_.OverwriteFromISR(rx);
    }

    return OM_OK;
  };

  System::Message::Topic<can_rx_item_t> cap_tp("can_cap", false);

  System::Message::Subscription<can_rx_item_t> cap_sub(cap_tp, rx_callback,
                                                       this);

  bsp_can_register_subscriber(this->param_.can, cap_tp.GetHandle(),
                              this->param_.index, 1);

  auto cap_thread = [](void *arg) {
    Cap *cap = (Cap *)arg;

    DECLARE_TOPIC(ui_tp, cap->ui_, "cap_ui", true);
    DECLARE_TOPIC(info_tp, cap->feedback_, "cap_info", true);

    DECLARE_SUBER(out_sub, cap->output, "cap_out");

    while (1) {
      /* 读取裁判系统信息 */
      if (!cap->Update()) {
        /* 一定时间长度内接收不到电容反馈值，使电容离线 */
        cap->Offline();
      }
      cap->PackUI();
      ui_tp.Publish();
      info_tp.Publish();

      out_sub.DumpData();
      cap->Control();

      /* 运行结束，等待下一次唤醒 */
      System::Thread::Sleep(10);
    }
  };

  THREAD_DECLEAR(this->thread_, cap_thread, 256, System::Thread::Medium, this);
}

bool Cap::Update() {
  can_rx_item_t rx;
  while (this->control_feedback_.Receive(&rx, 0)) {
    this->Decode(rx);
    this->online_ = 1;
    this->last_online_time_ = System::Thread::GetTick();
    return true;
  }

  if (System::Thread::GetTick() - this->last_online_time_ > 250) {
    return false;
  } else {
    return true;
  }
}

void Cap::Decode(can_rx_item_t &rx) {
  uint8_t *raw = rx.data;
  this->feedback_.input_volt_ =
      (float)((raw[1] << 8) | raw[0]) / (float)CAP_RES;
  this->feedback_.cap_volt_ = (float)((raw[3] << 8) | raw[2]) / (float)CAP_RES;
  this->feedback_.input_curr_ =
      (float)((raw[5] << 8) | raw[4]) / (float)CAP_RES;
  this->feedback_.target_power_ =
      (float)((raw[7] << 8) | raw[6]) / (float)CAP_RES;

  /* 更新电容状态和百分比 */
  this->feedback_.percentage_ = this->GetPercentage();
}

bool Cap::Control() {
  uint16_t pwr_lim = (uint16_t)(this->output.power_limit_ * CAP_RES);

  uint8_t data[8] = {0};
  data[0] = (pwr_lim >> 8) & 0xFF;
  data[1] = pwr_lim & 0xFF;

  return bsp_can_trans_packet(this->param_.can, DEV_CAP_CTRL_ID_BASE, data,
                              &this->mailbox_, 1) == BSP_OK;
}

bool Cap::Offline() {
  this->feedback_.cap_volt_ = 0;
  this->feedback_.input_curr_ = 0;
  this->feedback_.input_volt_ = 0;
  this->feedback_.target_power_ = 0;
  this->online_ = 0;

  return true;
}

void Cap::PackUI() {
  this->ui_.percentage = this->feedback_.percentage_;
  this->ui_.online = this->online_;
}

float Cap::GetPercentage() {
  const float c_max = this->feedback_.input_volt_ * this->feedback_.input_volt_;
  const float c_cap = this->feedback_.cap_volt_ * this->feedback_.cap_volt_;
  const float c_min = CAP_CUTOFF_VOLT * CAP_CUTOFF_VOLT;
  float percentage = (c_cap - c_min) / (c_max - c_min);
  clampf(&percentage, 0.0f, 1.0f);
  return percentage;
}
