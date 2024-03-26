/*
  自定义的类型
*/

#pragma once

#include <cmath>

#define M_DEG2RAD_MULT (0.01745329251f)
#define M_RAD2DEG_MULT (57.2957795131f)

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef M_2PI
#define M_2PI 6.28318530717958647692f
#endif

#ifndef M_1G
#define M_1G 9.80665f
#endif

namespace Component {
namespace Type {
class CycleValue {
 public:
  CycleValue& operator=(const CycleValue&) = default;

  static float Calculate(float value) {
    value = fmodf(value, M_2PI);
    if (value < 0) {
      value += M_2PI;
    }
    return value;
  }

  CycleValue(const float& value) : value_(Calculate(value)) {}

  CycleValue(const double& value)
      : value_(Calculate(static_cast<float>(value))) {}

  CycleValue(const CycleValue& value) : value_(value.value_) {
    while (value_ >= M_2PI) {
      value_ -= M_2PI;
    }

    while (value_ < 0) {
      value_ += M_2PI;
    }
  }

  CycleValue() : value_(0.0f) {}

  CycleValue operator+(const float& value) {
    return CycleValue(value + value_);
  }

  CycleValue operator+(const double& value) {
    return CycleValue(static_cast<float>(value) + value_);
  }

  CycleValue operator+(const CycleValue& value) {
    return CycleValue(value.value_ + value_);
  }

  CycleValue operator+=(const float& value) {
    value_ = Calculate(value + value_);

    return *this;
  }

  CycleValue operator+=(const double& value) {
    value_ = Calculate(static_cast<float>(value) + value_);

    return *this;
  }

  CycleValue operator+=(const CycleValue& value) {
    float ans = value.value_ + value_;
    while (ans >= M_2PI) {
      ans -= M_2PI;
    }

    while (ans < 0) {
      ans += M_2PI;
    }

    value_ = ans;

    return *this;
  }

  float operator-(const float& raw_value) {
    float value = Calculate(raw_value);
    float ans = value_ - value;
    while (ans >= M_PI) {
      ans -= M_2PI;
    }

    while (ans < -M_PI) {
      ans += M_2PI;
    }

    return ans;
  }

  float operator-(const double& raw_value) {
    float value = Calculate(static_cast<float>(raw_value));
    float ans = value_ - value;
    while (ans >= M_PI) {
      ans -= M_2PI;
    }

    while (ans < -M_PI) {
      ans += M_2PI;
    }

    return ans;
  }

  float operator-(const CycleValue& value) {
    float ans = value_ - value.value_;
    while (ans >= M_PI) {
      ans -= M_2PI;
    }

    while (ans < -M_PI) {
      ans += M_2PI;
    }

    return ans;
  }

  CycleValue operator-=(const float& value) {
    value_ = Calculate(value_ - value);

    return *this;
  }

  CycleValue operator-=(const double& value) {
    value_ = Calculate(value_ - static_cast<float>(value));

    return *this;
  }

  CycleValue operator-=(const CycleValue& value) {
    float ans = value_ - value.value_;
    while (ans >= M_2PI) {
      ans -= M_2PI;
    }

    while (ans < 0) {
      ans += M_2PI;
    }

    value_ = ans;

    return *this;
  }

  CycleValue operator-() { return CycleValue(M_2PI - value_); }

  operator float() { return this->value_; }

  CycleValue& operator=(const float& value) {
    value_ = Calculate(value);
    return *this;
  }

  CycleValue& operator=(const double& value) {
    value_ = static_cast<float>(value);
    return *this;
  }

  float Value() { return value_; }

 private:
  float value_;
};
/* 欧拉角（Euler angle） */
typedef struct {
  CycleValue yaw; /* 偏航角（Yaw angle） */
  CycleValue pit; /* 俯仰角（Pitch angle） */
  CycleValue rol; /* 翻滚角（Roll angle） */
} Eulr;

/* 四元数 */
typedef struct {
  float q0;
  float q1;
  float q2;
  float q3;
} Quaternion;

/* 移动向量 */
typedef struct {
  float vx; /* 前后平移 */
  float vy; /* 左右平移 */
  float wz; /* 转动 */
} MoveVector;

/* 二元素向量 */
typedef struct {
  float x;
  float y;
} Vector2;

/* 三元素向量 */
typedef struct {
  float x;
  float y;
  float z;
} Vector3;

class Position2 {
 public:
  static float Distance(const Position2& source, const Position2& target) {
    float dx = source.x_ - target.x_;
    float dy = source.y_ - target.y_;
    return sqrtf(dx * dx + dy * dy);
  }

  Position2() = default;

  Position2(float x, float y) : x_(x), y_(y) {}

  inline float GetLength() {
    return sqrtf(this->x_ * this->x_ + this->y_ * this->y_);
  }

  inline float GetAngle() { return atan2f(this->y_, this->x_); }

  const Position2 operator+(const Position2& pos) {
    return Position2(pos.x_ + this->x_, pos.y_ + this->y_);
  }

  float x_;

  float y_;
};

class Polar2 {
 public:
  operator Position2() {
    return Position2(this->distance_ * cosf(this->angle_),
                     this->distance_ * sinf(this->angle_));
  }

  Polar2() = default;

  Polar2(float angle, float distance) : angle_(angle), distance_(distance) {}

  Polar2(Position2& pos) : angle_(pos.GetAngle()), distance_(pos.GetLength()) {}

  float angle_;

  float distance_;
};

class Line {
 public:
  Line() = default;

  Line(const Position2& start, const Position2& end)
      : start_(start), end_(end) {}

  static const Position2 CrossPoint(const Line& l1, const Line& l2) {
    float x = 0.0f;
    float y = 0.0f;

    float k = (l1.start_.y_ - l1.end_.y_) / (l1.start_.x_ - l1.end_.x_);
    float b = l1.start_.y_ - k * l1.start_.x_;

    float k1 = (l2.start_.y_ - l2.end_.y_) / (l2.start_.x_ - l2.end_.x_);
    float b1 = l2.start_.y_ - k1 * l2.start_.x_;

    if (k1 != k) {
      x = (b1 - b) / (k - k1);
      y = k * x + b;

      return Position2(x, y);
    } else {
      return Position2(0, 0);
    }
  }

  const Position2 MiddlePoint() {
    return Position2((this->start_.x_ + this->end_.x_) / 2.0f,
                     (this->start_.y_ + this->end_.y_) / 2.0f);
  }

  float Length() {
    float dy = this->end_.y_ - this->start_.y_;
    float dx = this->end_.x_ - this->start_.x_;

    return sqrtf(dx * dx + dy * dy);
  }

  float Angle() {
    float dy = this->end_.y_ - this->start_.y_;
    float dx = this->end_.x_ - this->start_.x_;

    return atan2f(dy, dx);
  }

  Position2 start_, end_;
};

};  // namespace Type
}  // namespace Component
