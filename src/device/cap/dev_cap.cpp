#include "dev_cap.hpp"

#include "bsp_time.h"
#include "dev_referee.hpp"

#define CAP_RES (100.0f) /* 电容数据分辨率 */

using namespace Device;

Cap::Cap(Cap::Param &param) : param_(param), info_tp_("cap_info") {
  XB_ASSERT(param.cutoff_volt > 3.0f && param.cutoff_volt < 24.0f);

  out_.power_limit_ = 40.0f;

  auto rx_callback = [](Can::Pack &rx, Cap *cap) {
    rx.index -= cap->param_.index;

    if (rx.index == 0) {
      cap->control_feedback_.Overwrite(rx);
    }

    return true;
  };

  Message::Topic<Can::Pack> cap_tp("can_cap");
  cap_tp.RegisterCallback(rx_callback, this);

  Can::Subscribe(cap_tp, this->param_.can, this->param_.index, 1);

  auto ref_cb = [](Device::Referee::Data &ref, Cap *cap) {
    if (ref.status != Device::Referee::RUNNING) {
      cap->out_.power_limit_ = 40.0f;
    } else {
      cap->out_.power_limit_ = ref.robot_status.chassis_power_limit;
    }

    clampf(&cap->out_.power_limit_, 20.0f, 100.0f);

    return true;
  };

  Message::Topic<Device::Referee::Data>(
      Message::Topic<Device::Referee::Data>::Find("referee"))
      .RegisterCallback(ref_cb, this);

  auto cap_thread = [](Cap *cap) {
    uint32_t last_online_time = bsp_time_get_ms();
    while (1) {
      /* 读取裁判系统信息 */
      if (!cap->Update()) {
        /* 一定时间长度内接收不到电容反馈值，使电容离线 */
        cap->Offline();
      }
      cap->info_tp_.Publish(cap->info_);

      cap->Control();

      /* 运行结束，等待下一次唤醒 */
      cap->thread_.SleepUntil(100, last_online_time);
    }
  };

  this->thread_.Create(cap_thread, this, "cap_thread",
                       DEVICE_CAP_TASK_STACK_DEPTH, System::Thread::MEDIUM);
  System::Timer::Create(this->DrawUIStatic, this, 2300);

  System::Timer::Create(this->DrawUIDynamic, this, 200);
}

bool Cap::Update() {
  Can::Pack rx;
  while (this->control_feedback_.Receive(rx)) {
    this->Decode(rx);
    this->info_.online_ = 1;
    this->last_online_time_ = bsp_time_get_ms();
    return true;
  }

  if (bsp_time_get_ms() - this->last_online_time_ > 500) {
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
  this->info_.online_ = 0;

  return true;
}

float Cap::GetPercentage() {
  const float C_MAX = this->info_.input_volt_ * this->info_.input_volt_;
  const float C_CAP = this->info_.cap_volt_ * this->info_.cap_volt_;
  const float C_MIN = param_.cutoff_volt * param_.cutoff_volt;
  float percentage = (C_CAP - C_MIN) / (C_MAX - C_MIN);
  clampf(&percentage, 0.0f, 1.0f);
  return percentage;
}

void Cap::DrawUIStatic(Cap *cap) {
  cap->string_.Draw(
      "CE", Component::UI::UI_GRAPHIC_OP_ADD,
      Component::UI::UI_GRAPHIC_LAYER_CONST, Component::UI::UI_GREEN,
      UI_DEFAULT_WIDTH * 20, 80, UI_CHAR_DEFAULT_WIDTH * 2,
      static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.6f - 26.0f),
      static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2f + 10.0f),
      "CAP");

  if (cap->info_.online_) {
    cap->arc_.Draw("CP", Component::UI::UI_GRAPHIC_OP_ADD,
                   Component::UI::UI_GRAPHIC_LAYER_CAP, Component::UI::UI_GREEN,
                   0, static_cast<uint16_t>(cap->info_.percentage_ * 360.f),
                   UI_DEFAULT_WIDTH * 5,
                   static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.6f),
                   static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2f),
                   50, 50);
    Device::Referee::AddUI(cap->arc_);

  } else {
    cap->arc_.Draw("CP", Component::UI::UI_GRAPHIC_OP_ADD,
                   Component::UI::UI_GRAPHIC_LAYER_CAP,
                   Component::UI::UI_YELLOW, 0, 360, UI_DEFAULT_WIDTH * 5,
                   static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.6f),
                   static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2),
                   50, 50);
    Device::Referee::AddUI(cap->arc_);
  }
  Device::Referee::AddUI(cap->string_);
}

void Cap::DrawUIDynamic(Cap *cap) {
  if (cap->info_.online_) {
    cap->arc_.Draw("CP", Component::UI::UI_GRAPHIC_OP_REWRITE,
                   Component::UI::UI_GRAPHIC_LAYER_CAP, Component::UI::UI_GREEN,
                   0, static_cast<uint16_t>(cap->info_.percentage_ * 360.f),
                   UI_DEFAULT_WIDTH * 5,
                   static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.6f),
                   static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2f),
                   50, 50);
    Device::Referee::AddUI(cap->arc_);

  } else {
    cap->arc_.Draw("CP", Component::UI::UI_GRAPHIC_OP_REWRITE,
                   Component::UI::UI_GRAPHIC_LAYER_CAP,
                   Component::UI::UI_YELLOW, 0, 360, UI_DEFAULT_WIDTH * 5,
                   static_cast<uint16_t>(Device::Referee::UIGetWidth() * 0.6f),
                   static_cast<uint16_t>(Device::Referee::UIGetHeight() * 0.2),
                   50, 50);
    Device::Referee::AddUI(cap->arc_);
  }
}
