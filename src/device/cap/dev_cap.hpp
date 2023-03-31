#pragma once

#include <device.hpp>

#include "bsp_can.h"
#include "comp_type.hpp"
#include "comp_ui.hpp"
#include "dev_can.hpp"

#define DEV_CAP_FB_ID_BASE (0x211)
#define DEV_CAP_CTRL_ID_BASE (0x210)

namespace Device {
class Cap {
 public:
  typedef struct {
    float input_volt_;
    float cap_volt_;
    float input_curr_;
    float target_power_;
    float percentage_;
  } Info;

  typedef struct {
    float power_limit_;
  } Output;

  typedef struct {
    bsp_can_t can;
    uint32_t index;
  } Param;

  Cap(Param& param);

  bool Update();

  bool Control();

  bool Offline();

  void Decode(Can::Pack& rx);

  float GetPercentage();

  static void DrawUIStatic(Cap* cap);

  static void DrawUIDynamic(Cap* cap);

 private:
  Param param_;

  bool online_;

  float last_online_time_ = 0.0f;

  System::Queue<Can::Pack> control_feedback_ = System::Queue<Can::Pack>(1);

  System::Thread thread_;

  Message::Topic<Cap::Info> info_tp_;

  Cap::Info info_;

  Cap::Output out_;

  Component::UI::Ele ui_ele_data_;

  Component::UI::Str ui_string_data_;

  Component::UI::String string_;

  Component::UI::Arc arc_;
};
}  // namespace Device
