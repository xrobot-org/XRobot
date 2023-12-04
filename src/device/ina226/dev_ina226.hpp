#pragma once

#include "bsp_i2c.h"
#include "device.hpp"

namespace Device {
class Ina226 {
 public:
  typedef struct {
    uint8_t device_id;
    bsp_i2c_t i2c;
    float resistance;
  } Param;

  typedef struct {
    float shunt_volt;
    float bus_volt;
    float current;
    float power;
  } Info;

  Ina226(Param &param);

  static int Cali(Ina226 *ina, int argc, char **argv);

  void GetData();

  Param param_;
  Info info_;

  System::Database::Key<float> current_offset_;

  System::Term::Command<Ina226 *> cmd_;

  float max_current_;
  float current_lsb_;
  float power_lsb_;
  uint32_t cali_;

  System::Timer::TimerHandle timer_;

  Message::Topic<Info> info_tp_;
};
}  // namespace Device
