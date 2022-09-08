/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#pragma once

#include "comp_type.hpp"
#include "comp_utils.hpp"
#include "message.hpp"
#include "thread.hpp"

namespace Component {
class AHRS {
 public:
  AHRS();

  void Update();

  void GetEulr();

  float last_update_;
  float dt_;
  float now_;  // TODO:封装

  System::Thread thread_;

  DECLARE_PUBER(quat_, Type::Quaternion, "imu_quat", true);
  DECLARE_PUBER(eulr_, Type::Eulr, "imu_eulr", true);

  Type::Vector3 accl_;
  Type::Vector3 gyro_;
};
}  // namespace Component
