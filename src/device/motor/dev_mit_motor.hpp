#pragma once

#include <device.hpp>

#include "comp_ahrs.hpp"
#include "dev_can.hpp"
#include "dev_motor.hpp"

namespace Device {
class MitMotor : public BaseMotor {
 public:
  typedef struct {
    float kp;
    float kd;
    float def_speed;
    uint32_t id;
    bsp_can_t can;
    float max_error;
  } Param;

  MitMotor(const Param &param, const char *name);

  void Control(float output);

  bool Update();

  void Relax();

  void Decode(Can::Pack &rx);

  void SetCurrent(float current);

  void SetPos(float pos_error);

  Param param_;

  float current_ = 0.0f;

  System::Queue<Can::Pack> recv_ = System::Queue<Can::Pack>(1);

  static Message::Topic<Can::Pack> *mit_tp_[BSP_CAN_NUM];
};
}  // namespace Device
