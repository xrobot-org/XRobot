#pragma once

#include <stdbool.h>

#include "bsp_can.h"
#include "comp_type.hpp"
#include "comp_ui.hpp"
#include "dev.hpp"
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

  void Decode(CAN::Pack& rx);

  float GetPercentage();

  Param param_;

  bool online_;

  uint32_t last_online_time_;

  uint32_t mailbox_;

  System::Queue control_feedback_ = System::Queue(sizeof(CAN::Pack), 1);

  System::Thread thread_;

  Message::Topic<Cap::Info> info_tp_;

  Cap::Info info_;

  Cap::Output out_;
};
}  // namespace Device
