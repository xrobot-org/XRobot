#include "comp_triangle.hpp"

using namespace Component;

float Triangle::Supplementary(float angle) { return M_PI - angle; }

float Triangle::Reciprocal(float angle) { return M_PI / 2.0f - angle; }

float Triangle::InvSinThrm(float A, float a, float b) {
  return sinf(A) / a * b;
}

float Triangle::InvCosThrm(float a, float b, float c) {
  return acosf((a * a + b * b - c * c) / (2 * a * b));
}

float Triangle::SinThrm(float A, float a, float B) {
  return a / sinf(A) * sinf(B);
}

float Triangle::CosThrm(float a, float b, float C) {
  return sqrtf(a * a + b * b - 2 * a * b * cos(C));
}

bool Triangle::Slove() {
  uint8_t known_angle_num = 0, known_side_num = 0;

  for (uint8_t i = 0; i < 3; i++) {
    if (this->data_.angle[i] != 0) {
      known_angle_num++;
    }
    if (this->data_.side[i] != 0) {
      known_side_num++;
    }
  }

  if (known_angle_num + known_side_num < 3 || known_side_num == 0) {
    return false;
  }

  if (known_angle_num + known_side_num == 6) {
    return true;
  }

  switch (known_angle_num) {
    case 0:
      for (uint8_t i = 0; i < 3; i++) {
        this->data_.angle[i] =
            InvCosThrm(this->data_.side[(i + 1) % 3],
                       this->data_.side[(i + 2) % 3], this->data_.side[i]);
      }
      return true;
    case 1:
      for (uint8_t i = 0; i < 3; i++) {
        if (this->data_.angle[i] > 0) {
          std::array<uint8_t, 2> index = {static_cast<uint8_t>((i + 1) % 3),
                                          static_cast<uint8_t>((i + 2) % 3)};

          if (this->data_.side[i] == 0.0f) {
            this->data_.side[i] =
                CosThrm(this->data_.side[index[0]], this->data_.side[index[1]],
                        this->data_.angle[i]);
          }

          for (uint8_t t = 0; t < 2; t++) {
            if (this->data_.angle[index[t]] == 0.0f &&
                this->data_.side[index[t]] > 0) {
              this->data_.angle[index[t]] =
                  InvSinThrm(this->data_.angle[i], this->data_.side[i],
                             this->data_.side[index[t]]);
            } else if (this->data_.angle[index[t]] > 0 &&
                       this->data_.side[index[t]] == 0.0f) {
              this->data_.side[index[t]] =
                  SinThrm(this->data_.angle[i], this->data_.side[i],
                          this->data_.angle[index[t]]);
            }
          }

          return this->Slove();
        }
      }
      return false;
    case 2:
      for (uint8_t i = 0; i < 3; i++) {
        if (this->data_.angle[i] == 0.0f) {
          this->data_.angle[i] = Supplementary(this->data_.angle[(i + 1) % 3] +
                                               this->data_.angle[(i + 2) % 3]);
          break;
        }
      }
      return this->Slove();
    case 3: {
      uint8_t index = 0xff;
      for (uint8_t i = 0; i < 3; i++) {
        if (this->data_.side[i] != 0) {
          index = i;
          break;
        }
      }

      if (index > 2) {
        return false;
      }

      if (this->data_.side[(index + 1) % 3] == 0.0f) {
        this->data_.side[(index + 1) % 3] =
            SinThrm(this->data_.angle[index], this->data_.side[index],
                    this->data_.angle[(index + 1) % 3]);
      }
      if (this->data_.side[(index + 2) % 3] == 0.0f) {
        this->data_.side[(index + 2) % 3] =
            SinThrm(this->data_.angle[index], this->data_.side[index],
                    this->data_.angle[(index + 2) % 3]);
      }

      return true;
    }
    default:
      return false;
  }
}

void Triangle::Reset() {
  for (uint8_t i = 0; i < 3; i++) {
    this->data_.angle[i] = 0.0f;
    this->data_.side[i] = 0.0f;
  }
}
