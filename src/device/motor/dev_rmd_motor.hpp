#pragma once

#include <device.hpp>

#include "bsp_can.h"
#include "dev_can.hpp"
#include "dev_motor.hpp"

namespace Device {
class RMDMotor : public BaseMotor {
 public:
  typedef struct {
    uint8_t num;
    bsp_can_t can;
    bool reverse;
  } Param;

  RMDMotor(const Param& param, const char* name);

  RMDMotor(RMDMotor& motor);

  void Decode(Can::Pack& rx);

  bool Update();

  bool SendData();

  void Control(float output);

  void Offline();

  void Relax();

 private:
  Param param_;

  float output_;

  static uint8_t motor_tx_buff_[BSP_CAN_NUM][8];

  static uint8_t motor_tx_flag_[BSP_CAN_NUM];

  static uint8_t motor_tx_map_[BSP_CAN_NUM];

  System::Queue<Can::Pack> recv_ = System::Queue<Can::Pack>(1);
};
}  // namespace Device
