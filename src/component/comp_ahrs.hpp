/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#pragma once

#include <component.hpp>

namespace Component {
class AHRS {
 public:
  AHRS();

  void Update();

  void GetEulr();

  static int ShowCMD(AHRS *ahrs, int argc, char *argv[]);

  float last_update_;
  float dt_;
  float now_;

  System::Thread thread_;

  Message::Topic<Type::Quaternion> quat_tp_;

  Message::Topic<Type::Eulr> eulr_tp_;

  Type::Quaternion quat_;
  Type::Eulr eulr_;

  Type::Vector3 accl_;
  Type::Vector3 gyro_;

  System::Term::Command<AHRS *> cmd_;

  System::Semaphore accl_ready_;
  System::Semaphore gyro_ready_;
  System::Semaphore ready_;
};
}  // namespace Component
