/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#pragma once

#include "comp_type.hpp"
#include "comp_utils.hpp"
#include "thread.hpp"

namespace Component {
class AHRS {
 public:
  typedef enum { GIMBAL, CHASSIS } Source;

  AHRS(Source source);

  void Update();

  void GetEulr();

  float last_update_;
  float dt_;
  float now_;  // TODO:封装

  Component::Type::Quaternion quat_;
  Component::Type::Vector3 accl_, gyro_;
  Component::Type::Eulr eulr_;

  Source source_;

  System::Thread thread_;
};
}  // namespace Component
