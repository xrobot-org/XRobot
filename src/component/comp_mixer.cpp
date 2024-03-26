/*
  混合器
*/

#include "comp_mixer.hpp"

using namespace Component;

Mixer::Mixer(Mixer::Mode mode) : mode_(mode) {
  switch (mode) {
    case MECANUM:
      this->len_ = 4;
      break;

    case PARLFIX4:
      this->len_ = 4;
      break;

    case PARLFIX2:
      this->len_ = 2;
      break;

    case OMNICROSS:
      this->len_ = 4;
      break;

    case OMNIPLUS:
      this->len_ = 4;
      break;

    case SINGLE:
      this->len_ = 1;
      break;

    case NONE:
      this->len_ = 0;
      return;

    default:
      XB_ASSERT(false);
  }
}

bool Mixer::Apply(Component::Type::MoveVector &move_vec, float *out) {
  switch (this->mode_) {
    case MECANUM:
      XB_ASSERT(this->len_ == 4);
      out[0] = move_vec.vx - move_vec.vy + move_vec.wz;
      out[1] = move_vec.vx + move_vec.vy + move_vec.wz;
      out[2] = -move_vec.vx + move_vec.vy + move_vec.wz;
      out[3] = -move_vec.vx - move_vec.vy + move_vec.wz;
      break;

    case PARLFIX4:
      XB_ASSERT(this->len_ == 4);
      out[0] = -move_vec.vy;
      out[1] = move_vec.vy;
      out[2] = move_vec.vy;
      out[3] = -move_vec.vy;
      break;

    case PARLFIX2:
      XB_ASSERT(this->len_ == 2);
      out[0] = -move_vec.vx;
      out[1] = move_vec.vx;
      break;

    case SINGLE:
      XB_ASSERT(this->len_ == 1);
      out[0] = move_vec.vy;
      break;

    case OMNICROSS:
      XB_ASSERT(this->len_ == 4);
      out[0] = move_vec.vx - move_vec.vy + move_vec.wz;
      out[1] = move_vec.vx + move_vec.vy + move_vec.wz;
      out[2] = -move_vec.vx + move_vec.vy + move_vec.wz;
      out[3] = -move_vec.vx - move_vec.vy + move_vec.wz;
      break;
    case OMNIPLUS:
      break;

    case NONE:
      break;

    default:
      break;
  }

  float abs_max = 0.f;
  for (size_t i = 0; i < this->len_; i++) {
    const float ABS_VAL = fabsf(out[i]);
    abs_max = (ABS_VAL > abs_max) ? ABS_VAL : abs_max;
  }
  if (abs_max > 1.f) {
    for (size_t i = 0; i < this->len_; i++) {
      out[i] /= abs_max;
    }
  }

  return 0;
}
