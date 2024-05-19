#include "dev_cap.hpp"

#include "bsp_time.h"
#include "dev_referee.hpp"

#define CAP_RES (100.0f) /* 电容数据分辨率 */
#define V_MAX (23.0f)
#define V_MIN (16.0f)
using namespace Device;

Cap::Cap(Cap::Param &param) : param_(param), info_tp_("cap_info") {
  out_.power_limit_ = 40.0f;

  auto rx_callback = [](Can::Pack &rx, Cap *cap) {
    cap->control_feedback_.Overwrite(rx);
    cap->can_recv_.Post();
    return true;
  };

  Message::Topic<Can::Pack> cap_tp("can_cap");
  cap_tp.RegisterCallback(rx_callback, this);

  Can::Subscribe(cap_tp, this->param_.can, 0x600, 19);

  auto ref_cb = [](Device::Referee::Data &ref, Cap *cap) {
    if (ref.status != Device::Referee::RUNNING) {
      cap->out_.power_limit_ = 40.0f;
    } else {
      float power_buff_percentage = 0.0f;
      if (ref.power_heat.chassis_pwr_buff >= 40) {
        power_buff_percentage = 1.0f;
      } else {
        power_buff_percentage =
            (ref.power_heat.chassis_pwr_buff - 20.0f) / 20.0f;
      }
      clampf(&power_buff_percentage, 0.0f, 1.0f);
      cap->out_.power_limit_ =
          static_cast<float>(ref.robot_status.chassis_power_limit) - 3.0f +
          10.0f * power_buff_percentage;
    }
    clampf(&cap->out_.power_limit_, 0.0f, 150.0f);
    cap->power_limit_update_.Post();
    return true;
  };

  Message::Topic<Device::Referee::Data>(
      Message::Topic<Device::Referee::Data>::Find("referee"))
      .RegisterCallback(ref_cb, this);

  auto cap_thread = [](Cap *cap) {
    while (1) {
      /* 读取裁判系统信息 */
      if (!cap->Update()) {
        /* 一定时间长度内接收不到电容反馈值，使电容离线 */
        cap->Offline();
      }
      cap->InstructUpdata();
      cap->Control(INSTRUCT);
      cap->Control(OUTPUT_VOLT);
      cap->Control(OUTPUT);
      if (cap->power_limit_update_.Wait(10)) {
        cap->Control(POWER_LIMIT);
      }

      if (cap->can_recv_.Wait(10)) {
        cap->Update();
      }
      cap->info_tp_.Publish(cap->info_);
    }
  };

  this->thread_.Create(cap_thread, this, "cap_thread",
                       DEVICE_CAP_TASK_STACK_DEPTH, System::Thread::MEDIUM);
  System::Timer::Create(this->DrawUIStatic, this, 2300);

  System::Timer::Create(this->DrawUIDynamic, this, 200);
}

bool Cap::InstructUpdata() {
  if (info_.online_ == 1) {
    instruct_ = 2;
  } else {
    instruct_ = 0;
  }
  return true;
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
  switch (rx.index) {
    case OUTPUT: {
      this->info_.output_power_ =
          static_cast<float>((raw[0] << 8) | raw[1]) / 100;
      this->info_.cap_volt_ = static_cast<float>((raw[2] << 8) | raw[3]) / 100;
      this->info_.output_curr_ =
          static_cast<float>((raw[4] << 8) | raw[5]) / 100;
      break;
    }
    case INSTRUCT: {
      this->info_.cap_instruct_ = static_cast<uint16_t>((raw[0] << 8) | raw[1]);
      break;
    }
    case POWER_LIMIT: {
      this->info_.target_power_ =
          static_cast<float>((raw[0] << 8) | raw[1]) / 100;
      break;
    }
    case OUTPUT_VOLT: {
      this->info_.cap_volt_max_ =
          static_cast<float>((raw[0] << 8) | raw[1]) / 100;
      break;
    }
  }
  this->info_.online_ = true;
  /* 更新电容状态和百分比 */
  this->info_.percentage_ = this->GetPercentage();
}

bool Cap::Control(CanID can_id_) {
  Can::Pack tx_buff;
  tx_buff.index = can_id_;
  switch (tx_buff.index) {
    case OUTPUT_VOLT: {
      uint16_t cap_volt = 2300;
      tx_buff.data[0] = (cap_volt >> 8) & 0xFF;
      tx_buff.data[1] = cap_volt & 0xFF;
      Can::SendStdPack(this->param_.can, tx_buff);
      break;
    }
    case OUTPUT: {
      Can::SendStdRemotePack(this->param_.can, tx_buff);
      break;
    }
    case INSTRUCT: {
      tx_buff.data[0] = (instruct_ >> 8) & 0XFF;
      tx_buff.data[1] = instruct_ & 0XFF;
      Can::SendStdPack(this->param_.can, tx_buff);
      break;
    }
    case POWER_LIMIT: {
      uint16_t pwr_lim = static_cast<uint16_t>(this->out_.power_limit_ * 100);
      tx_buff.data[0] = (pwr_lim >> 8) & 0xFF;
      tx_buff.data[1] = pwr_lim & 0xFF;
      Can::SendStdPack(this->param_.can, tx_buff);
      break;
    }
  }
  return true;
}

bool Cap::Offline() {
  this->info_.cap_volt_ = 0;
  this->info_.output_curr_ = 0;
  this->info_.target_power_ = 0;
  this->info_.online_ = 0;
  this->info_.cap_instruct_ = 0;
  this->info_.cap_volt_max_ = 0;
  return true;
}

float Cap::GetPercentage() {
  const float C_MAX = V_MAX * V_MAX;
  const float C_CAP = this->info_.cap_volt_ * this->info_.cap_volt_;
  const float C_MIN = V_MIN * V_MIN;
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
