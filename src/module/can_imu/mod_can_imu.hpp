#pragma once

#include "dev_can.hpp"

namespace Module {
class CanIMU {
 public:
  CanIMU();

  void SendAccl();

  void SendGyro();

  void SendEulr();

  Component::Type::Eulr eulr_;
  Component::Type::Vector3 gyro_;
  Component::Type::Vector3 accl_;

  uint32_t mailbox_;

  System::Thread thread_;
};
}  // namespace Module
