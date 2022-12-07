#pragma once

#include "dev_can.hpp"
#include "dev_can_imu.hpp"

#if IMU_USE_IN_WEARLAB
#include "wearlab.hpp"
#endif

namespace Module {
class CanIMU {
 public:
  CanIMU();

  void SendAccl();

  void SendGyro();

  void SendEulr();

  void SendQuat();

  Component::Type::Eulr eulr_;
  Component::Type::Quaternion quat_;
  Component::Type::Vector3 gyro_;
  Component::Type::Vector3 accl_;

  System::Thread thread_;
};
}  // namespace Module
