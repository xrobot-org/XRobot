#include "dev_cap.hpp"

#include "bsp_time.h"

#define CAP_RES (100.0f) /* 电容数据分辨率 */

#define CAP_CUTOFF_VOLT \
  13.0f /* 电容截止电压，未接升压模块时要高于电调最低工作电压 */

using namespace Device;

Cap::Cap(Cap::Param &param) : param_(param), info_tp_("cap_info") {
  auto rx_callback = [](Can::Pack &rx, Cap *cap) {
    rx.index -= cap->param_.index;

    if (rx.index == 0) {
      cap->control_feedback_.OverwriteFromISR(rx);
    }

    return true;
  };

  Message::Topic<Can::Pack> cap_tp("can_cap");
  cap_tp.RegisterCallback(rx_callback, this);

  Can::Subscribe(cap_tp, this->param_.can, this->param_.index, 1);

  auto cap_thread = [](Cap *cap) {
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
      cap->thread_.SleepUntil(10);
    }
  };

  this->thread_.Create(cap_thread, this, "cap_thread", 256,
                       System::Thread::Medium);
}

bool Cap::Update() {
  Can::Pack rx;
  while (this->control_feedback_.Receive(rx, 0)) {
    this->Decode(rx);
    this->online_ = 1;
    this->last_online_time_ = bsp_time_get();
    return true;
  }

  if (bsp_time_get() - this->last_online_time_ > 0.25f) {
    return false;
  } else {
    return true;
  }
}

void Cap::Decode(Can::Pack &rx) {
  uint8_t *raw = rx.data;
  this->info_.input_volt_ =
      static_cast<float>((raw[1] << 8) | raw[0]) / CAP_RES;
  this->info_.cap_volt_ = static_cast<float>((raw[3] << 8) | raw[2]) / CAP_RES;
  this->info_.input_curr_ =
      static_cast<float>((raw[5] << 8) | raw[4]) / CAP_RES;
  this->info_.target_power_ =
      static_cast<float>((raw[7] << 8) | raw[6]) / CAP_RES;

  /* 更新电容状态和百分比 */
  this->info_.percentage_ = this->GetPercentage();
}

bool Cap::Control() {
  uint16_t pwr_lim = static_cast<uint16_t>(this->out_.power_limit_ * CAP_RES);

  Can::Pack tx_buff;

  tx_buff.index = DEV_CAP_CTRL_ID_BASE;

  tx_buff.data[0] = (pwr_lim >> 8) & 0xFF;
  tx_buff.data[1] = pwr_lim & 0xFF;

  return Can::SendStdPack(this->param_.can, tx_buff);
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
  const float C_MAX = this->info_.input_volt_ * this->info_.input_volt_;
  const float C_CAP = this->info_.cap_volt_ * this->info_.cap_volt_;
  const float C_MIN = CAP_CUTOFF_VOLT * CAP_CUTOFF_VOLT;
  float percentage = (C_CAP - C_MIN) / (C_MAX - C_MIN);
  clampf(&percentage, 0.0f, 1.0f);
  return percentage;
}
