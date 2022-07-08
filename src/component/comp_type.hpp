/*
  自定义的类型
*/

#pragma once
namespace Component {
class Type {
 public:
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
};
}  // namespace Component
