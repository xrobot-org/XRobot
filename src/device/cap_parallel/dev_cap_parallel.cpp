#include "dev_cap_parallel.hpp"

#include "bsp_time.h"
#include "dev_referee.hpp"

#define CAP_RES (100.0f) /* 电容数据分辨率 */
#define V_MAX (23.0f)
#define V_MIN (16.0f)
#define CAP_V_MAX (30.f)
#define CAP_I_MAX (20.f)
using namespace Device;

CapParallel::CapParallel(CapParallel::Param &param)
    : param_(param), info_tp_("cap_info"), ctrl_lock_(true) {
  out_.power_limit_ = 40.0f;
  auto rx_callback = [](Can::Pack &rx, CapParallel *cap) {
    cap->control_feedback_.Overwrite(rx);
    return true;
  };

  Message::Topic<Can::Pack> cap_tp("can_cap");
  cap_tp.RegisterCallback(rx_callback, this);

  Can::Subscribe(cap_tp, this->param_.can, 0x2E, 3);

  auto ref_cb = [](Device::Referee::Data &ref, CapParallel *cap) {
    if (ref.status != Device::Referee::RUNNING) {
      cap->out_.power_limit_ = 40.0f;
    } else {
      float power_buff_percentage = 0.0f;
      if (ref.power_heat.chassis_pwr_buff >= 40) {
        power_buff_percentage = 1.0f;
      } else {
        power_buff_percentage =
            (static_cast<float>(ref.power_heat.chassis_pwr_buff) - 20.0f) /
            20.0f;
      }
      clampf(&power_buff_percentage, 0.0f, 1.0f);
      cap->out_.power_limit_ =
          static_cast<float>(ref.robot_status.chassis_power_limit) - 3.0f +
          100.0f * power_buff_percentage;
    }
    clampf(&cap->out_.power_limit_, 0.0f, 250.0f);
    return true;
  };

  Message::Topic<Device::Referee::Data>(
      Message::Topic<Device::Referee::Data>::Find("referee"))
      .RegisterCallback(ref_cb, this);

  auto event_callback = [](CapEvent event, CapParallel *cap) {
    cap->ctrl_lock_.Wait(UINT32_MAX);
    switch (event) {
      case SET_MODE_OPEN:
        cap->SetMode(OPEN);
        break;
      case SET_MODE_OFF:
        cap->SetMode(OFF);
        break;
      default:
        break;
    }
  };

  auto cap_thread = [](CapParallel *cap) {
    while (1) {
      /* 读取裁判系统信息 */
      if (!cap->Update()) {
        /* 一定时间长度内接收不到电容反馈值，使电容离线 */
        cap->Offline();
      }
      cap->InstructUpdata();
      cap->Control(CAPCONTROL);
      cap->Control(POWERBUF);
      cap->info_tp_.Publish(cap->info_);
    }
  };
  Component::CMD::RegisterEvent<CapParallel *, CapEvent>(
      event_callback, this, this->param_.EVENT_MAP);
  this->thread_.Create(cap_thread, this, "cap_thread", 1024,
                       System::Thread::MEDIUM);
  System::Timer::Create(this->DrawUIStatic, this, 2300);

  System::Timer::Create(this->DrawUIDynamic, this, 200);
}

bool CapParallel::InstructUpdata() {
  if (this->info_.online_ == 1 && this->raw_ref_.robot_status.remain_hp != 0 &&
      this->mode_ == OPEN) {
    this->info_.cap_instruct_ = 1;
  } else {
    this->info_.cap_instruct_ = 0;
  }
  return true;
}
bool CapParallel::Update() {
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
void CapParallel::GetCapState() {
  this->capstate_.OverVlot = this->cap_state_ & 1;
  this->capstate_.OverCurrent = this->cap_state_ & 2;
  this->capstate_.UnderVlot = this->cap_state_ & 4;
  this->capstate_.RefUnderVlot = this->cap_state_ & 8;
}
void CapParallel::Decode(Can::Pack &rx) {
  uint8_t *raw = rx.data;
  this->info_.cap_volt_ =
      static_cast<float>((raw[0] << 8) | raw[1]) * CAP_V_MAX / 32000;
  this->info_.cap_cur_ =
      static_cast<float>((raw[2] << 8) | raw[3]) * CAP_I_MAX / 32000;
  this->cap_state_ = (raw[4] << 8) | raw[5];
  this->info_.online_ = true;
  /* 更新电容状态和百分比 */
  this->info_.percentage_ = this->GetPercentage();
}

bool CapParallel::Control(CanId can_id_) {
  Can::Pack tx_buff;
  tx_buff.index = can_id_;
  switch (tx_buff.index) {
    case POWERBUF: {
      uint16_t pwr_buff =
          static_cast<uint16_t>(this->raw_ref_.power_heat.chassis_pwr_buff);
      tx_buff.data[0] = pwr_buff >> 8;
      tx_buff.data[1] = pwr_buff;
      break;
    }
    case CAPCONTROL: {
      uint16_t pwr_lim = static_cast<uint16_t>(
          this->raw_ref_.robot_status.chassis_power_limit);
      uint16_t cap_out_power_lim =
          static_cast<uint16_t>(this->out_.power_limit_);
      uint16_t cap_input_power_lim =
          static_cast<uint16_t>(this->input_.power_limit_);
      tx_buff.data[0] = pwr_lim >> 8;
      tx_buff.data[1] = pwr_lim;
      tx_buff.data[2] = cap_out_power_lim >> 8;
      tx_buff.data[3] = cap_out_power_lim;
      tx_buff.data[4] = cap_input_power_lim >> 8;
      tx_buff.data[5] = cap_input_power_lim;
      tx_buff.data[6] = this->info_.cap_instruct_ >> 8;
      tx_buff.data[7] = this->info_.cap_instruct_;
      break;
    }
  }
  Can::SendStdPack(this->param_.can, tx_buff);
  return true;
}
bool CapParallel::Offline() {
  this->info_.cap_volt_ = 0;
  this->info_.target_power_ = 0;
  this->info_.online_ = 0;
  this->info_.cap_instruct_ = 0;
  return true;
}
void CapParallel::SetMode(CapMode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }
  this->mode_ = mode;
}
float CapParallel::GetPercentage() {
  const float C_MAX = V_MAX * V_MAX;
  const float C_CAP = this->info_.cap_volt_ * this->info_.cap_volt_;
  const float C_MIN = V_MIN * V_MIN;
  float percentage = (C_CAP - C_MIN) / (C_MAX - C_MIN);
  clampf(&percentage, 0.0f, 1.0f);
  return percentage;
}

void CapParallel::DrawUIStatic(CapParallel *cap) {
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

void CapParallel::DrawUIDynamic(CapParallel *cap) {
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
