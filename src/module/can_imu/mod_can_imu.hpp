#pragma once
#include <module.hpp>

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

 private:
#if IMU_SEND_EULR
  Component::Type::Eulr eulr_;
#endif
#if IMU_SEND_QUAT
  Component::Type::Quaternion quat_;
#endif
#if IMU_SEND_GYRO
  Component::Type::Vector3 gyro_;
#endif
#if IMU_SEND_ACCL
  Component::Type::Vector3 accl_;
#endif

  System::Thread thread_;
};
}  // namespace Module
