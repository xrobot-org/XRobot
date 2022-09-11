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

  void Decode(can_rx_item_t& rx);

  float GetPercentage();

  Param param_;

  bool online_;

  uint32_t last_online_time_;

  uint32_t mailbox_;

  System::Queue control_feedback_ = System::Queue(sizeof(can_rx_item_t), 1);

  System::Thread thread_;

  DECLARE_PUBER(info_, Cap::Info, "cap_info", true);

  Cap::Output out_;
};
}  // namespace Device
