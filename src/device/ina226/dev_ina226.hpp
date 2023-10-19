#pragma once

#include "bsp_i2c.h"
#include "device.hpp"

namespace Device {
class Ina226 {
 public:
  typedef struct {
    uint8_t device_id;
    bsp_i2c_t i2c;
    uint16_t cali;
  } Param;

  typedef struct {
    float shunt_volt;
    float bus_volt;
    float current;
    float power;
  } Info;

  Ina226(Param &param);

  Param param_;
  Info info_;

  Message::Topic<Info> info_tp_;
};
}  // namespace Device
