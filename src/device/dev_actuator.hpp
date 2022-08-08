#pragma once

#include <stdint.h>

#include "comp_cf.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"
#include "dev_motor.hpp"

namespace Device {
class SpeedActuator {
 public:
  typedef struct {
    Component::PID::Param speed;
    float in_cutoff_freq;
    float out_cutoff_freq;
  } Param;

  SpeedActuator(Param& param, float sample_freq);

  float Calculation(float setpoint, float feedback, float dt);

  void Reset();

  Component::PID pid_;

  Component::LowPassFilter2p in_;
  Component::LowPassFilter2p out_;
};

class PosActuator {
 public:
  typedef struct {
    Component::PID::Param speed;
    Component::PID::Param position;
    float in_cutoff_freq;
    float out_cutoff_freq;
  } Param;

  PosActuator(Param& param, float sample_freq);

  float Calculation(float setpoint, float speed_fb, float pos_fb, float dt);

  void Reset();

  Component::PID pid_speed_;
  Component::PID pid_position_;

  Component::LowPassFilter2p in_speed_;
  Component::LowPassFilter2p in_position_;

  Component::LowPassFilter2p out_;
};
}  // namespace Device
