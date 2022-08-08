#pragma once

#include "bsp_can.h"
#include "comp_ahrs.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"
#include "dev_motor.hpp"

namespace Device {
class MitMotor : public Motor {
 public:
  typedef struct {
    float kp;
    float kd;
    float def_speed;
    uint32_t id;
    bsp_can_t can;
  } Param;

  MitMotor(const Param &param, const char *name);

  void Control(float output);

  bool Update();

  void Relax();

  void Decode(can_rx_item_t &rx);

  void Set(float pos_error);

  Param param_;

  System::Queue recv_;

  uint32_t mailbox_;

  static System::Message::Topic<can_rx_item_t> *mit_tp[BSP_CAN_NUM];
};
}  // namespace Device
