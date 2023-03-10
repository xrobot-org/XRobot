/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#pragma once

#include <device.hpp>

namespace Device {
class AHRS {
 public:
  AHRS();

  void Update();

  void GetEulr();

  static int ShowCMD(AHRS *ahrs, int argc, char **argv);

 private:
  float last_update_;
  float dt_;
  float now_;

  System::Thread thread_;

  Message::Topic<Component::Type::Quaternion> quat_tp_;

  Message::Topic<Component::Type::Eulr> eulr_tp_;

  Component::Type::Quaternion quat_;
  Component::Type::Eulr eulr_;

  Component::Type::Vector3 accl_;
  Component::Type::Vector3 gyro_;

  System::Term::Command<AHRS *> cmd_;

  System::Semaphore accl_ready_;
  System::Semaphore gyro_ready_;
  System::Semaphore ready_;
};
}  // namespace Device
