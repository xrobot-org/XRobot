#pragma once

#include "bsp_i2c.h"
#include "comp_filter.hpp"
#include "device.hpp"

namespace Device {
class MT6701 {
 public:
  typedef struct {
    uint8_t address;
    bsp_i2c_t i2c;
    float mech_zero;
    uint32_t timeout;
  } Param;

  MT6701(Param& param) : param_(param) {
    auto task_fun = [](MT6701* mt6701) { mt6701->GetAngle(); };
    System::Timer::Create(task_fun, this, param.timeout);
  }

  float GetAngle() {
    bsp_i2c_mem_read(param_.i2c, param_.address, 0x03, i2c_buff_, 2, true);
    uint16_t raw_data = i2c_buff_[0] << 6 | i2c_buff_[1] >> 2;
    angle_ = static_cast<float>(raw_data) / 16384.0f * M_2PI + param_.mech_zero;
    return angle_;
  }

  Param param_;
  uint8_t i2c_buff_[2];
  Component::Type::CycleValue angle_;
};
}  // namespace Device
