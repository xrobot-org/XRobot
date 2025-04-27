#pragma once

#include <component.hpp>

#include "comp_cf.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"

namespace Component {
class ActuatorStallDetect {
 public:
  typedef struct {
    float speed_thld;        /* 速度阈值 */
    float current_thld;      /* 电流阈值 */
    float stop_current_thld; /* 静止电流阈值 */
    float temp_thld;         /* 温度阈值 */
    float timeout;           /* 检测时间 */
  } Param;

  ActuatorStallDetect(Param& param) : param_(param) {}

  bool Calculate(float speed_fb, float current_fb, float temp_fb, float dt) {
    if (temp_fb >= param_.temp_thld) {
      return true;
    }

    if ((fabsf(current_fb) >= param_.current_thld &&
         fabsf(speed_fb) >= param_.speed_thld) ||
        fabsf(current_fb) >= param_.current_thld) {
      time_ += dt;
    } else {
      time_ = 0;
    }

    if (time_ >= param_.timeout) {
      return true;
    } else {
      return false;
    }
  }

 private:
  Param& param_;
  float time_ = 0.0f;
};
class SpeedActuator {
 public:
  typedef struct {
    Component::PID::Param speed;
    float in_cutoff_freq;
    float out_cutoff_freq;
  } Param;

  SpeedActuator(Param& param, float sample_freq);

  float Calculate(float setpoint, float feedback, float dt);

  void Reset();

 private:
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

  float Calculate(float setpoint, float speed_fb, float pos_fb, float dt);

  float SpeedCalculate(float setpoint, float feedback, float dt);

  float GetLastError() { return pid_speed_.GetError(); }

  void Reset();

 private:
  Component::PID pid_speed_;
  Component::PID pid_position_;

  Component::LowPassFilter2p in_speed_;
  Component::LowPassFilter2p in_position_;

  Component::LowPassFilter2p out_;
};
}  // namespace Component
