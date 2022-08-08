/*
  自定义的类型
*/

#pragma once

#include "math.h"

namespace Component {
namespace Type {
/* 欧拉角（Euler angle） */
typedef struct {
  float yaw; /* 偏航角（Yaw angle） */
  float pit; /* 俯仰角（Pitch angle） */
  float rol; /* 翻滚角（Roll angle） */
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
    float x;
    float y;

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
