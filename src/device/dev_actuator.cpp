#include "dev_actuator.hpp"

using namespace Device;

Actuator::Actuator(PosParam& param, float sample_freq, const char* name)
    : in_pos_(sample_freq, param.in_cutoff_freq),
      in_speed_(sample_freq, param.in_cutoff_freq),
      out_(sample_freq, param.out_cutoff_freq),
      motor_(param.motor, name) {
  this->speed_ =
      (Component::PID*)System::Memory::Malloc(sizeof(Component::PID));
  this->pos_ = (Component::PID*)System::Memory::Malloc(sizeof(Component::PID));

  new (this->speed_) Component::PID(param.speed, sample_freq);
  new (this->pos_) Component::PID(param.pos, sample_freq);

  this->reverse_ = param.reverse;
}

Actuator::Actuator(SpeedParam& param, float sample_freq, const char* name)
    : in_pos_(sample_freq, param.in_cutoff_freq),
      in_speed_(sample_freq, param.in_cutoff_freq),
      out_(sample_freq, param.out_cutoff_freq),
      motor_(param.motor, name) {
  this->speed_ =
      (Component::PID*)System::Memory::Malloc(sizeof(Component::PID));
  this->pos_ = NULL;
  new (this->speed_) Component::PID(param.speed, sample_freq);

  this->reverse_ = param.reverse;
}

bool Actuator::Control(float setpoint, float dt) {
  float out;

  ASSERT(!this->pos_);

  out = this->speed_->Calculate(setpoint,
                                this->motor_.feedback_.rotational_speed, dt);

  if (this->reverse_) {
    this->motor_.Control(-out);
  } else {
    this->motor_.Control(out);
  }

  this->motor_.AddData();

  return true;
}

bool Actuator::Control(float setpoint, float pos_fb, float speed_fb, float dt) {
  ASSERT(this->pos_);

  float speed_setpoint = this->pos_->Calculate(setpoint, pos_fb, dt);

  float out = this->speed_->Calculate(speed_setpoint, speed_fb, dt);

  if (this->reverse_) {
    this->motor_.Control(-out);
  } else {
    this->motor_.Control(out);
  }

  this->motor_.AddData();

  return true;
}

bool Actuator::UpdateFeedback() {
  this->motor_.Update();
  if (this->reverse_) {
    circle_reverse(&(this->motor_.feedback_.rotor_abs_angle));
  }

  this->motor_.feedback_.rotational_speed =
      this->in_speed_.Apply(this->motor_.feedback_.rotational_speed);

  this->motor_.feedback_.rotor_abs_angle =
      this->in_pos_.Apply(this->motor_.feedback_.rotor_abs_angle);

  return true;
}

void Actuator::Reset() {
  this->in_pos_.Reset(0.0f);
  this->in_speed_.Reset(0.0f);
  this->out_.Reset(0.0f);
  this->speed_->Reset();
  if (this->pos_) this->pos_->Reset();
}

void Actuator::Relax() {
  this->motor_.Control(0.0f);
  this->motor_.AddData();
}

float Actuator::GetPos() { return this->motor_.feedback_.rotor_abs_angle; }

float Actuator::GetSpeed() { return this->motor_.feedback_.rotational_speed; }

LimitedActuator::LimitedActuator(SpeedParam& speed_param,
                                 LimitParam& limit_param, float sample_freq,
                                 const char* name)
    : Actuator(speed_param, sample_freq, name),
      in_load_(limit_param.filter_k),
      limit_param_(limit_param),
      default_k_(speed_param.speed.k) {}

bool LimitedActuator::Control(float* setpoint, LimitedActuator* actuator,
                              uint8_t len, float limit_rate, float dt) {
  float load_sum = 0.0f;

  for (uint8_t i = 0; i < len; i++) {
    load_sum += actuator[i].load_;
  }

  if (load_sum == 0.0f) {
    for (uint8_t i = 0; i < len; i++) {
      actuator[i].speed_->SetK(actuator[i].default_k_);
      actuator[i].Actuator::Control(setpoint[i], dt);
    }
    return false;
  }

  for (uint8_t i = 0; i < len; i++) {
    float k = (pow(actuator[i].load_ / load_sum * len, 2) - 1) * len + 1;
    clampf(&k, actuator[i].limit_param_.min_k_percent,
           actuator[i].limit_param_.max_k_percent);
    actuator[i].speed_->SetK(k * actuator[i].default_k_);

    float out;

    out = actuator[i].speed_->Calculate(
              setpoint[i], actuator[i].motor_.feedback_.rotational_speed, dt) *
          limit_rate;

    if (actuator[i].reverse_) {
      actuator[i].motor_.Control(-out);
    } else {
      actuator[i].motor_.Control(out);
    }

    actuator[i].motor_.AddData();
  }

  return true;
}

bool LimitedActuator::UpdateFeedback() {
  this->Actuator::UpdateFeedback();
  float load;
  if (fabs(this->motor_.feedback_.torque_current) <
          this->limit_param_.min_current ||
      this->motor_.output_ == 0) {
    load = this->limit_param_.max_load;
  } else {
    load = this->motor_.feedback_.rotational_speed /
           (this->motor_.feedback_.torque_current * 1000.0f);
    clampf(&load, 0, this->limit_param_.max_load);
  }

  this->load_ = this->in_load_.Apply(this->limit_param_.max_load - load);

  return true;
}

void LimitedActuator::Reset() {
  this->speed_->SetK(this->default_k_);
  this->in_load_.Reset(0.0f);
  this->Actuator::Reset();
}
