#pragma once

#include <device.hpp>

#include "dev_referee.hpp"

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
    bool online_;
  } Info;

  typedef struct {
    float power_limit_;
  } Output;

  typedef struct {
  } Param;

  Cap(Param& param);

 private:
  Message::Topic<Cap::Info> info_tp_;

  Cap::Info info_;
};
}  // namespace Device
