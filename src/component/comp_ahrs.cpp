/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#include "comp_ahrs.hpp"

#include <string.h>

#include "comp_utils.hpp"

#define BETA_IMU (0.033f)
#define BETA_AHRS (0.041f)

using namespace Component;

AHRS::AHRS() : quat_tp_("imu_quat"), eulr_tp_("imu_eulr") {
  this->quat_.q0 = 1.0f;
  this->quat_.q1 = 0.0f;
  this->quat_.q2 = 0.0f;
  this->quat_.q3 = 0.0f;

  auto ahrs_thread = [](void* arg) {
    AHRS* ahrs = static_cast<AHRS*>(arg);

    Message::Subscriber accl_sub("imu_accl", ahrs->accl_);
    Message::Subscriber gyro_sub("imu_gyro", ahrs->gyro_);

    while (1) {
      accl_sub.DumpData();
      gyro_sub.DumpData();

      ahrs->Update();

      /* 根据解析出来的四元数计算欧拉角 */
      ahrs->GetEulr();
      /* 发布数据 */
      ahrs->quat_tp_.Publish(ahrs->quat_);
      ahrs->eulr_tp_.Publish(ahrs->eulr_);

      /* 运行结束，等待下一次唤醒 */
      ahrs->thread_.Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, ahrs_thread, 256, System::Thread::High, this);
}

void AHRS::Update() {
  this->now_ = System::Thread::GetTick() / 1000.0f;

  this->dt_ = this->now_ - this->last_update_;
  this->last_update_ = this->now_;

  float ax = this->accl_.x;
  float ay = this->accl_.y;
  float az = this->accl_.z;

  float gx = this->gyro_.x;
  float gy = this->gyro_.y;
  float gz = this->gyro_.z;

  float recip_norm;
  float s0, s1, s2, s3;
  float q_dot1, q_dot2, q_dot3, q_dot4;
  float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1, q2q2,
      q3q3;

  /* Rate of change of quaternion from gyroscope */
  q_dot1 =
      0.5f * (-this->quat_.q1 * gx - this->quat_.q2 * gy - this->quat_.q3 * gz);
  q_dot2 =
      0.5f * (this->quat_.q0 * gx + this->quat_.q2 * gz - this->quat_.q3 * gy);
  q_dot3 =
      0.5f * (this->quat_.q0 * gy - this->quat_.q1 * gz + this->quat_.q3 * gx);
  q_dot4 =
      0.5f * (this->quat_.q0 * gz + this->quat_.q1 * gy - this->quat_.q2 * gx);

  /* Compute feedback only if accelerometer measurement valid (avoids NaN in
   * accelerometer normalisation) */
  if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
    /* Normalise accelerometer measurement */
    recip_norm = inv_sqrtf(ax * ax + ay * ay + az * az);
    ax *= recip_norm;
    ay *= recip_norm;
    az *= recip_norm;

    /* Auxiliary variables to avoid repeated arithmetic */
    _2q0 = 2.0f * this->quat_.q0;
    _2q1 = 2.0f * this->quat_.q1;
    _2q2 = 2.0f * this->quat_.q2;
    _2q3 = 2.0f * this->quat_.q3;
    _4q0 = 4.0f * this->quat_.q0;
    _4q1 = 4.0f * this->quat_.q1;
    _4q2 = 4.0f * this->quat_.q2;
    _8q1 = 8.0f * this->quat_.q1;
    _8q2 = 8.0f * this->quat_.q2;
    q0q0 = this->quat_.q0 * this->quat_.q0;
    q1q1 = this->quat_.q1 * this->quat_.q1;
    q2q2 = this->quat_.q2 * this->quat_.q2;
    q3q3 = this->quat_.q3 * this->quat_.q3;

    /* Gradient decent algorithm corrective step */
    s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
    s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * this->quat_.q1 - _2q0 * ay -
         _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
    s2 = 4.0f * q0q0 * this->quat_.q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay -
         _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
    s3 = 4.0f * q1q1 * this->quat_.q3 - _2q1 * ax +
         4.0f * q2q2 * this->quat_.q3 - _2q2 * ay;

    /* normalise step magnitude */
    recip_norm = inv_sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);

    s0 *= recip_norm;
    s1 *= recip_norm;
    s2 *= recip_norm;
    s3 *= recip_norm;

    /* Apply feedback step */
    q_dot1 -= BETA_IMU * s0;
    q_dot2 -= BETA_IMU * s1;
    q_dot3 -= BETA_IMU * s2;
    q_dot4 -= BETA_IMU * s3;
  }

  /* Integrate rate of change of quaternion to yield quaternion */
  this->quat_.q0 += q_dot1 * this->dt_;
  this->quat_.q1 += q_dot2 * this->dt_;
  this->quat_.q2 += q_dot3 * this->dt_;
  this->quat_.q3 += q_dot4 * this->dt_;

  /* Normalise quaternion */
  recip_norm = inv_sqrtf(
      this->quat_.q0 * this->quat_.q0 + this->quat_.q1 * this->quat_.q1 +
      this->quat_.q2 * this->quat_.q2 + this->quat_.q3 * this->quat_.q3);
  this->quat_.q0 *= recip_norm;
  this->quat_.q1 *= recip_norm;
  this->quat_.q2 *= recip_norm;
  this->quat_.q3 *= recip_norm;
}

void AHRS::GetEulr() {
  const float sinr_cosp = 2.0f * (this->quat_.q0 * this->quat_.q1 +
                                  this->quat_.q2 * this->quat_.q3);
  const float cosr_cosp = 1.0f - 2.0f * (this->quat_.q1 * this->quat_.q1 +
                                         this->quat_.q2 * this->quat_.q2);
  this->eulr_.pit = atan2f(sinr_cosp, cosr_cosp);

  const float sinp = 2.0f * (this->quat_.q0 * this->quat_.q2 -
                             this->quat_.q3 * this->quat_.q1);

  if (fabsf(sinp) >= 1.0f)
    this->eulr_.rol = copysignf(M_PI / 2.0f, sinp);
  else
    this->eulr_.rol = asinf(sinp);

  const float siny_cosp = 2.0f * (this->quat_.q0 * this->quat_.q3 +
                                  this->quat_.q1 * this->quat_.q2);
  const float cosy_cosp = 1.0f - 2.0f * (this->quat_.q2 * this->quat_.q2 +
                                         this->quat_.q3 * this->quat_.q3);
  this->eulr_.yaw = atan2f(siny_cosp, cosy_cosp);
}
