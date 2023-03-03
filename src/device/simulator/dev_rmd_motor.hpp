#pragma once

#include "dev_motor.hpp"

namespace Device {
class RMDMotor : public BaseMotor {
 public:
  typedef struct {
  } Param;

  RMDMotor(const Param& param, const char* name);

  void Control(float output);

  bool Update();
};
}  // namespace Device
