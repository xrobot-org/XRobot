#include "comp_actuator.hpp"

using namespace Component;

SpeedActuator::SpeedActuator(Param& param, float sample_freq)
    : pid_(param.speed, sample_freq),
      in_(sample_freq, param.in_cutoff_freq),
      out_(sample_freq, param.out_cutoff_freq) {}

float SpeedActuator::Calculation(float setpoint, float feedback, float dt) {
  feedback = this->in_.Apply(feedback);

  float out = this->pid_.Calculate(setpoint, feedback, dt);

  this->out_.Apply(out);

  return out;
}

void SpeedActuator::Reset() {
  this->in_.Reset(0.0f);
  this->out_.Reset(0.0f);
  this->pid_.Reset();
}

PosActuator::PosActuator(Param& param, float sample_freq)
    : pid_speed_(param.speed, sample_freq),
      pid_position_(param.position, sample_freq),
      in_speed_(sample_freq, param.in_cutoff_freq),
      in_position_(sample_freq, param.in_cutoff_freq),
      out_(sample_freq, param.out_cutoff_freq) {}

float PosActuator::Calculation(float setpoint, float speed_fb, float pos_fb,
                               float dt) {
  speed_fb = this->in_speed_.Apply(speed_fb);
  pos_fb = this->in_position_.Apply(pos_fb);

  float speed_setpoint =
      this->pid_position_.Calculate(setpoint, pos_fb, speed_fb, dt);

  float out = this->pid_speed_.Calculate(speed_setpoint, speed_fb, dt);

  this->out_.Apply(out);

  return out;
}

void PosActuator::Reset() {
  this->in_speed_.Reset(0.0f);
  this->in_position_.Reset(0.0f);
  this->out_.Reset(0.0f);
  this->pid_speed_.Reset();
  this->pid_position_.Reset();
}
