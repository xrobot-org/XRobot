#pragma once

#include <stdint.h>

#include "comp_cf.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"
#include "dev_motor.hpp"

namespace Device {
class Actuator {
 public:
  typedef struct {
    Component::PID::Param speed;
    Component::PID::Param pos;
    float in_cutoff_freq;
    float out_cutoff_freq;
    Device::Motor::Param motor;
    bool reverse;
  } PosParam;

  typedef struct {
    Component::PID::Param speed;
    float in_cutoff_freq;
    float out_cutoff_freq;
    Device::Motor::Param motor;
    bool reverse;
  } SpeedParam;

  Actuator(PosParam& param, float sample_freq, const char* name);

  Actuator(SpeedParam& param, float sample_freq, const char* name);

  bool Control(float setpoint, float dt);

  bool Control(float setpoint, float pos_fb, float speed_fb, float dt);

  bool UpdateFeedback();

  void Reset();

  void Relax();

  float GetPos();

  float GetSpeed();

  Component::PID *speed_, *pos_;

  Component::LowPassFilter2p in_pos_;
  Component::LowPassFilter2p in_speed_;
  Component::LowPassFilter2p out_;

  Device::Motor motor_;

  bool reverse_;
};

class LimitedActuator : public Actuator {
 public:
  typedef struct {
    float min_k_percent;
    float max_k_percent;
    float max_load;
    float min_current;
    float filter_k;
  } LimitParam;

  LimitedActuator(SpeedParam& speed_param, LimitParam& limit_param,
                  float sample_freq, const char* name);

  static bool Control(float* setpoint, LimitedActuator* actuator, uint8_t len,
                      float limit_rate, float dt);

  bool UpdateFeedback();

  void Reset();

  Component::LowPassFilter in_load_;

  LimitParam& limit_param_;

  float load_;

  const float default_k_;
};

class FeedForwardActuator : public Actuator {
 public:
  FeedForwardActuator(Component::SecOrderFunction::Param& ff_param,
                      PosParam& pos_param, float sample_freq, const char* name);

  bool Control(float setpoint, float pos_fb, float speed_fb, float ff_fb,
               float dt);

  Component::SecOrderFunction ff_;
};
}  // namespace Device
