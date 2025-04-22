/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#pragma once

#include <device.hpp>
#include <comp_filter.hpp>

namespace Device {
class AHRS {
 public:
  AHRS();

  void Update();

  void GetEulr();

  void GetABSAccl();





  static int ShowCMD(AHRS *ahrs, int argc, char **argv);

 private:
  uint64_t last_wakeup_ = 0;
  uint64_t now_ = 0;
  float dt_ = 0.0f;

  float LowPassFilter(float sample,float dt, float cut_freq_,float last_out_);

  System::Thread thread_;
  //Component::LowPassFilter dfilter_;
  Message::Topic<Component::Type::Quaternion> quat_tp_;

  Message::Topic<Component::Type::Eulr> eulr_tp_;

  Message::Topic<Component::Type::Vector3> accl_abs_tp_;

  Component::Type::Quaternion quat_{};
  Component::Type::Eulr eulr_{};

  Component::Type::Vector3 accl_{};
  Component::Type::Vector3 gyro_{};
  Component::Type::Vector3 accl_abs_{};


  System::Term::Command<AHRS *> cmd_;

  System::Semaphore accl_ready_;
  System::Semaphore gyro_ready_;
  System::Semaphore ready_;
};
}  // namespace Device
