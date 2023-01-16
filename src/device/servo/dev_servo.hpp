#pragma once

#include <device.hpp>

namespace Device {
class Servo {
  typedef enum {
    SERVO_A = 0,
    SERVO_B,
    SERVO_C,
    SERVO_D,
    SERVO_E,
    SERVO_F,
    SERVO_G,
    SERVO_NUM,
  } Channel;

  Servo(Channel ch, float max_angle);

  bool Start();

  bool Set(float angle);

  bool Stop();

  Channel channel_;

  float max_angle_;
};
}  // namespace Device
