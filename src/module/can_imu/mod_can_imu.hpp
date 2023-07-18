#pragma once
#include <module.hpp>

#include "dev_can.hpp"
#include "dev_can_imu.hpp"

namespace Module {
class CanIMU {
 public:
  CanIMU();

  void SendAccl();

  void SendGyro();

  void SendEulr();

  void SendQuat();

  static int SetCMD(CanIMU* imu, int argc, char** argv);

 private:
  Component::Type::Eulr eulr_;
  Component::Type::Quaternion quat_;
  Component::Type::Vector3 gyro_;
  Component::Type::Vector3 accl_;

  System::Database::Key<bool> enable_eulr_;
  System::Database::Key<bool> enable_quat_;
  System::Database::Key<bool> enable_gyro_;
  System::Database::Key<bool> enable_accl_;

  System::Database::Key<uint32_t> delay_;

  System::Database::Key<uint32_t> can_id_;

  System::Term::Command<CanIMU*> cmd_;

  System::Thread thread_;
};
}  // namespace Module
