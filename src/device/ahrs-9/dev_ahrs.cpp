/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#include "dev_ahrs.hpp"

#include "bsp_time.h"

#define BETA_AHRS (0.05f)
#define BETA_IMU (0.033f)
using namespace Device;

AHRS::AHRS()
    : quat_tp_("imu_quat"),
      eulr_tp_("imu_eulr"),
      cmd_(this, AHRS::ShowCMD, "AHRS", System::Term::DevDir()),
      gyro_ready_(false) {
  this->quat_.q0 = -1.0f;
  this->quat_.q1 = 0.0f;
  this->quat_.q2 = 0.0f;
  this->quat_.q3 = 0.0f;

  auto ahrs_thread = [](AHRS *ahrs) {
    Message::Subscriber<Component::Type::Vector3> accl_sub("imu_accl");
    Message::Subscriber<Component::Type::Vector3> gyro_sub("imu_gyro");
    Message::Subscriber<Component::Type::Vector3> magn_sub("magn");

    System::Thread::Sleep(10);

    auto gyro_cb = [](Component::Type::Vector3 &gyro, AHRS *ahrs) {
      static_cast<void>(gyro);

      ahrs->gyro_ready_.Post();

      return true;
    };

    (Message::Topic<Component::Type::Vector3>(
         Message::Topic<Component::Type::Vector3>::Find("imu_gyro")))
        .RegisterCallback(gyro_cb, ahrs);

    float yaw = -atan2f(ahrs->magn_.y, ahrs->magn_.x);

    if ((ahrs->magn_.x == 0.0f) && (ahrs->magn_.y == 0.0f) &&
        (ahrs->magn_.z == 0.0f)) {
      ahrs->quat_.q0 = 0.800884545f;
      ahrs->quat_.q1 = 0.00862364192f;
      ahrs->quat_.q2 = -0.00283267116f;
      ahrs->quat_.q3 = 0.598749936f;
    } else if ((yaw < (M_PI / 2.0f)) || (yaw > 0.0f)) {
      ahrs->quat_.q0 = 0.997458339f;
      ahrs->quat_.q1 = 0.000336312107f;
      ahrs->quat_.q2 = -0.0057230792f;
      ahrs->quat_.q3 = 0.0740156546f;
    } else if ((yaw < M_PI) || (yaw > (M_PI / 2.0f))) {
      ahrs->quat_.q0 = 0.800884545f;
      ahrs->quat_.q1 = 0.00862364192f;
      ahrs->quat_.q2 = -0.00283267116f;
      ahrs->quat_.q3 = 0.598749936f;
    } else if ((yaw < 90.0f) || (yaw > M_PI)) {
      ahrs->quat_.q0 = 0.800884545f;
      ahrs->quat_.q1 = 0.00862364192f;
      ahrs->quat_.q2 = -0.00283267116f;
      ahrs->quat_.q3 = 0.598749936f;
    } else if ((yaw < 90.0f) || (yaw > 0.0f)) {
      ahrs->quat_.q0 = 0.800884545f;
      ahrs->quat_.q1 = 0.00862364192f;
      ahrs->quat_.q2 = -0.00283267116f;
      ahrs->quat_.q3 = 0.598749936f;
    }

    ahrs->last_wakeup_ = bsp_time_get();

    while (1) {
      if (ahrs->gyro_ready_.Wait(UINT32_MAX)) {
        gyro_sub.DumpData(ahrs->gyro_);
        accl_sub.DumpData(ahrs->accl_);
        magn_sub.DumpData(ahrs->magn_);
      }
      if (ahrs->magn_.x == 0 && ahrs->magn_.y == 0 && ahrs->magn_.z == 0) {
        ahrs->UpdateWithoutMagn();
      } else {
        ahrs->Update();
      }

      /* 根据解析出来的四元数计算欧拉角 */
      ahrs->GetEulr();
      /* 发布数据 */
      ahrs->quat_tp_.Publish(ahrs->quat_);
      ahrs->eulr_tp_.Publish(ahrs->eulr_);
    }
  };

  this->thread_.Create(ahrs_thread, this, "ahrs_thread",
                       DEVICE_AHRS_TASK_STACK_DEPTH, System::Thread::HIGH);
}

int AHRS::ShowCMD(AHRS *ahrs, int argc, char **argv) {
  if (argc == 1) {
    printf("[show] [time] [delay] 在time时间内每隔delay打印一次数据\r\n");
  } else if (argc == 4) {
    if (strcmp(argv[1], "show") == 0) {
      int time = std::stoi(argv[2]);
      int delay = std::stoi(argv[3]);

      if (delay > 1000) {
        delay = 1000;
      }

      if (delay < 2) {
        delay = 2;
      }

      while (time > delay) {
        printf("pitch:%f roll:%f yaw:%f", ahrs->eulr_.pit.Value(),
               ahrs->eulr_.rol.Value(), ahrs->eulr_.yaw.Value());
        System::Thread::Sleep(delay);
        ms_clear_line();
        time -= delay;
      }
      printf("pitch:%f roll:%f yaw:%f\r\n", ahrs->eulr_.pit.Value(),
             ahrs->eulr_.rol.Value(), ahrs->eulr_.yaw.Value());
    }
  }

  return 0;
}

static float recip_norm;
static float s0, s1, s2, s3;
static float q_dot1, q_dot2, q_dot3, q_dot4;
static float hx, hy;
static float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1,
    _2q2, _2q3, _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2,
    q2q3, q3q3;
static float q_2q0, q_2q1, q_2q2, q_2q3, q_4q0, q_4q1, q_4q2, q_8q1, q_8q2;

void AHRS::Update() {
  this->now_ = bsp_time_get();
  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  float mx = this->magn_.x;
  float my = this->magn_.y;
  float mz = this->magn_.z;

  float ax = this->accl_.x;
  float ay = this->accl_.y;
  float az = this->accl_.z;

  float gx = this->gyro_.x;
  float gy = this->gyro_.y;
  float gz = this->gyro_.z;

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

    /* Normalise magnetometer measurement */
    recip_norm = inv_sqrtf(mx * mx + my * my + mz * mz);
    mx *= recip_norm;
    my *= recip_norm;
    mz *= recip_norm;

    /* Auxiliary variables to avoid repeated arithmetic */
    _2q0mx = 2.0f * this->quat_.q0 * mx;
    _2q0my = 2.0f * this->quat_.q0 * my;
    _2q0mz = 2.0f * this->quat_.q0 * mz;
    _2q1mx = 2.0f * this->quat_.q1 * mx;
    _2q0 = 2.0f * this->quat_.q0;
    _2q1 = 2.0f * this->quat_.q1;
    _2q2 = 2.0f * this->quat_.q2;
    _2q3 = 2.0f * this->quat_.q3;
    _2q0q2 = 2.0f * this->quat_.q0 * this->quat_.q2;
    _2q2q3 = 2.0f * this->quat_.q2 * this->quat_.q3;
    q0q0 = this->quat_.q0 * this->quat_.q0;
    q0q1 = this->quat_.q0 * this->quat_.q1;
    q0q2 = this->quat_.q0 * this->quat_.q2;
    q0q3 = this->quat_.q0 * this->quat_.q3;
    q1q1 = this->quat_.q1 * this->quat_.q1;
    q1q2 = this->quat_.q1 * this->quat_.q2;
    q1q3 = this->quat_.q1 * this->quat_.q3;
    q2q2 = this->quat_.q2 * this->quat_.q2;
    q2q3 = this->quat_.q2 * this->quat_.q3;
    q3q3 = this->quat_.q3 * this->quat_.q3;

    /* Reference direction of Earth's magnetic field */
    hx = mx * q0q0 - _2q0my * this->quat_.q3 + _2q0mz * this->quat_.q2 +
         mx * q1q1 + _2q1 * my * this->quat_.q2 + _2q1 * mz * this->quat_.q3 -
         mx * q2q2 - mx * q3q3;
    hy = _2q0mx * this->quat_.q3 + my * q0q0 - _2q0mz * this->quat_.q1 +
         _2q1mx * this->quat_.q2 - my * q1q1 + my * q2q2 +
         _2q2 * mz * this->quat_.q3 - my * q3q3;
    _2bx = sqrtf(hx * hx + hy * hy);
    _2bz = -_2q0mx * this->quat_.q2 + _2q0my * this->quat_.q1 + mz * q0q0 +
           _2q1mx * this->quat_.q3 - mz * q1q1 + _2q2 * my * this->quat_.q3 -
           mz * q2q2 + mz * q3q3;
    _4bx = 2.0f * _2bx;
    _4bz = 2.0f * _2bz;

    /* Gradient decent algorithm corrective step */
    s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q1 * (2.0f * q0q1 + _2q2q3 - ay) -
         _2bz * this->quat_.q2 *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (-_2bx * this->quat_.q3 + _2bz * this->quat_.q1) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         _2bx * this->quat_.q2 *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q0 * (2.0f * q0q1 + _2q2q3 - ay) -
         4.0f * this->quat_.q1 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) +
         _2bz * this->quat_.q3 *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (_2bx * this->quat_.q2 + _2bz * this->quat_.q0) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         (_2bx * this->quat_.q3 - _4bz * this->quat_.q1) *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q3 * (2.0f * q0q1 + _2q2q3 - ay) -
         4.0f * this->quat_.q2 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) +
         (-_4bx * this->quat_.q2 - _2bz * this->quat_.q0) *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (_2bx * this->quat_.q1 + _2bz * this->quat_.q3) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         (_2bx * this->quat_.q0 - _4bz * this->quat_.q2) *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q2 * (2.0f * q0q1 + _2q2q3 - ay) +
         (-_4bx * this->quat_.q3 + _2bz * this->quat_.q1) *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (-_2bx * this->quat_.q0 + _2bz * this->quat_.q2) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         _2bx * this->quat_.q1 *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    /* normalise step magnitude */
    recip_norm = inv_sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
    s0 *= recip_norm;
    s1 *= recip_norm;
    s2 *= recip_norm;
    s3 *= recip_norm;

    /* Apply feedback step */
    q_dot1 -= BETA_AHRS * s0;
    q_dot2 -= BETA_AHRS * s1;
    q_dot3 -= BETA_AHRS * s2;
    q_dot4 -= BETA_AHRS * s3;
  }

  /* Integrate rate of change of quaternion to yield quaternion */
  this->quat_.q0 += q_dot1 * dt_;
  this->quat_.q1 += q_dot2 * dt_;
  this->quat_.q2 += q_dot3 * dt_;
  this->quat_.q3 += q_dot4 * dt_;

  /* Normalise quaternion */
  recip_norm = inv_sqrtf(
      this->quat_.q0 * this->quat_.q0 + this->quat_.q1 * this->quat_.q1 +
      this->quat_.q2 * this->quat_.q2 + this->quat_.q3 * this->quat_.q3);
  this->quat_.q0 *= recip_norm;
  this->quat_.q1 *= recip_norm;
  this->quat_.q2 *= recip_norm;
  this->quat_.q3 *= recip_norm;
}

void AHRS::UpdateWithoutMagn() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  float ax = this->accl_.x;
  float ay = this->accl_.y;
  float az = this->accl_.z;

  float gx = this->gyro_.x;
  float gy = this->gyro_.y;
  float gz = this->gyro_.z;

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
    q_2q0 = 2.0f * this->quat_.q0;
    q_2q1 = 2.0f * this->quat_.q1;
    q_2q2 = 2.0f * this->quat_.q2;
    q_2q3 = 2.0f * this->quat_.q3;
    q_4q0 = 4.0f * this->quat_.q0;
    q_4q1 = 4.0f * this->quat_.q1;
    q_4q2 = 4.0f * this->quat_.q2;
    q_8q1 = 8.0f * this->quat_.q1;
    q_8q2 = 8.0f * this->quat_.q2;
    q0q0 = this->quat_.q0 * this->quat_.q0;
    q1q1 = this->quat_.q1 * this->quat_.q1;
    q2q2 = this->quat_.q2 * this->quat_.q2;
    q3q3 = this->quat_.q3 * this->quat_.q3;

    /* Gradient decent algorithm corrective step */
    s0 = q_4q0 * q2q2 + q_2q2 * ax + q_4q0 * q1q1 - q_2q1 * ay;
    s1 = q_4q1 * q3q3 - q_2q3 * ax + 4.0f * q0q0 * this->quat_.q1 - q_2q0 * ay -
         q_4q1 + q_8q1 * q1q1 + q_8q1 * q2q2 + q_4q1 * az;
    s2 = 4.0f * q0q0 * this->quat_.q2 + q_2q0 * ax + q_4q2 * q3q3 - q_2q3 * ay -
         q_4q2 + q_8q2 * q1q1 + q_8q2 * q2q2 + q_4q2 * az;
    s3 = 4.0f * q1q1 * this->quat_.q3 - q_2q1 * ax +
         4.0f * q2q2 * this->quat_.q3 - q_2q2 * ay;

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
  const float SINR_COSP = 2.0f * (this->quat_.q0 * this->quat_.q1 +
                                  this->quat_.q2 * this->quat_.q3);
  const float COSR_COSP = 1.0f - 2.0f * (this->quat_.q1 * this->quat_.q1 +
                                         this->quat_.q2 * this->quat_.q2);
  this->eulr_.pit = atan2f(SINR_COSP, COSR_COSP);

  const float SINP = 2.0f * (this->quat_.q0 * this->quat_.q2 -
                             this->quat_.q3 * this->quat_.q1);

  if (fabsf(SINP) >= 1.0f) {
    this->eulr_.rol = copysignf(M_PI / 2.0f, SINP);
  } else {
    this->eulr_.rol = asinf(SINP);
  }

  const float SINY_COSP = 2.0f * (this->quat_.q0 * this->quat_.q3 +
                                  this->quat_.q1 * this->quat_.q2);
  const float COSY_COSP = 1.0f - 2.0f * (this->quat_.q2 * this->quat_.q2 +
                                         this->quat_.q3 * this->quat_.q3);
  this->eulr_.yaw = atan2f(SINY_COSP, COSY_COSP);
}
