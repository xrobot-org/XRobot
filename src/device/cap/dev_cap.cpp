#include "dev_cap.hpp"

#include <stdbool.h>
#include <string.h>

#include "comp_utils.hpp"
#include "om.h"

#define CAP_RES (100) /* 电容数据分辨率 */

#define CAP_CUTOFF_VOLT \
  13.0f /* 电容截止电压，未接升压模块时要高于电调最低工作电压 */

using namespace Device;

Cap::Cap(Cap::Param &param) : param_(param), info_tp_("cap_info") {
  auto rx_callback = [](CAN::Pack &rx, Cap *cap) {
    rx.index -= cap->param_.index;

    if (rx.index == 0) {
      cap->control_feedback_.OverwriteFromISR(&rx);
    }

    return true;
  };

  Message::Topic<CAN::Pack> cap_tp("can_cap");
  cap_tp.RegisterCallback(rx_callback, this);

  CAN::Subscribe(cap_tp, this->param_.can, this->param_.index, 1);

  auto cap_thread = [](void *arg) {
    Cap *cap = static_cast<Cap *>(arg);

    Message::Subscriber out_sub("cap_out", cap->out_);

    while (1) {
      /* 读取裁判系统信息 */
      if (!cap->Update()) {
        /* 一定时间长度内接收不到电容反馈值，使电容离线 */
        cap->Offline();
      }
      cap->info_tp_.Publish(cap->info_);

      out_sub.DumpData();
      cap->Control();

      /* 运行结束，等待下一次唤醒 */
      System::Thread::Sleep(10);
    }
  };

  THREAD_DECLEAR(this->thread_, cap_thread, 256, System::Thread::Medium, this);
}

bool Cap::Update() {
  CAN::Pack rx;
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

void Cap::Decode(CAN::Pack &rx) {
  uint8_t *raw = rx.data;
  this->info_.input_volt_ = (float)((raw[1] << 8) | raw[0]) / (float)CAP_RES;
  this->info_.cap_volt_ = (float)((raw[3] << 8) | raw[2]) / (float)CAP_RES;
  this->info_.input_curr_ = (float)((raw[5] << 8) | raw[4]) / (float)CAP_RES;
  this->info_.target_power_ = (float)((raw[7] << 8) | raw[6]) / (float)CAP_RES;

  /* 更新电容状态和百分比 */
  this->info_.percentage_ = this->GetPercentage();
}

bool Cap::Control() {
  uint16_t pwr_lim = (uint16_t)(this->out_.power_limit_ * CAP_RES);

  CAN::Pack tx_buff;

  tx_buff.index = DEV_CAP_CTRL_ID_BASE;

  tx_buff.data[0] = (pwr_lim >> 8) & 0xFF;
  tx_buff.data[1] = pwr_lim & 0xFF;

  return CAN::SendPack(this->param_.can, tx_buff);
}

bool Cap::Offline() {
  this->info_.cap_volt_ = 0;
  this->info_.input_curr_ = 0;
  this->info_.input_volt_ = 0;
  this->info_.target_power_ = 0;
  this->online_ = 0;

  return true;
}

float Cap::GetPercentage() {
  const float c_max = this->info_.input_volt_ * this->info_.input_volt_;
  const float c_cap = this->info_.cap_volt_ * this->info_.cap_volt_;
  const float c_min = CAP_CUTOFF_VOLT * CAP_CUTOFF_VOLT;
  float percentage = (c_cap - c_min) / (c_max - c_min);
  clampf(&percentage, 0.0f, 1.0f);
  return percentage;
}
