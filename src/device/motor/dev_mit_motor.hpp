#pragma once

#include "comp_ahrs.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"
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

  void Decode(CAN::Pack &rx);

  void SetCurrent(float current);

  void SetPos(float pos_error);

  Param param_;

  float current_ = 0.0f;

  System::Queue recv_;

  uint32_t mailbox_;

  static Message::Topic<CAN::Pack> *mit_tp[BSP_CAN_NUM];
};
}  // namespace Device
