#pragma once

#include <stdbool.h>

#include "bsp_can.h"
#include "comp_type.hpp"
#include "comp_ui.hpp"
#include "dev.hpp"

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
  } Feedback;

  typedef struct {
    float power_limit_;
  } Output;

  typedef struct {
    bsp_can_t can;
    uint32_t index;
  } Param;

  Param param_;
  Output output;
  Feedback feedback_;
  bool online_;
  uint32_t last_online_time_;
  uint32_t mailbox_;
  ui_cap_t ui_;

  System::Queue control_feedback_ = System::Queue(sizeof(can_rx_item_t), 1);

  System::Thread thread_;

  Cap(Param& param);

  bool Update();

  bool Control();

  bool Offline();

  void Decode(can_rx_item_t& rx);

  float GetPercentage();

  void PackUI();
};
}  // namespace Device
