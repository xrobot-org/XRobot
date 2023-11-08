/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#include "dev_ahrs.hpp"

#include "bsp_time.h"

#define BETA_IMU (0.033f)
using namespace Device;

AHRS::AHRS()
    : quat_tp_("imu_quat"),
      eulr_tp_("imu_eulr"),
      cmd_(this, AHRS::ShowCMD, "AHRS", System::Term::DevDir()),
      accl_ready_(false),
      gyro_ready_(false),
      ready_(false) {
  this->quat_.q0 = -1.0f;
  this->quat_.q1 = 0.0f;
  this->quat_.q2 = 0.0f;
  this->quat_.q3 = 0.0f;

  auto ahrs_thread = [](AHRS *ahrs) {
    Message::Subscriber<Component::Type::Vector3> accl_sub("imu_accl");
    Message::Subscriber<Component::Type::Vector3> gyro_sub("imu_gyro");

    auto accl_cb = [](Component::Type::Vector3 &accl, AHRS *ahrs) {
      static_cast<void>(accl);

      ahrs->ready_.Post();

      ahrs->accl_ready_.Post();

      return true;
    };

    auto gyro_cb = [](Component::Type::Vector3 &gyro, AHRS *ahrs) {
      static_cast<void>(gyro);

      ahrs->ready_.Post();

      ahrs->gyro_ready_.Post();

      return true;
    };

    (Message::Topic<Component::Type::Vector3>(
         Message::Topic<Component::Type::Vector3>::Find("imu_accl")))
        .RegisterCallback(accl_cb, ahrs);

    (Message::Topic<Component::Type::Vector3>(
         Message::Topic<Component::Type::Vector3>::Find("imu_gyro")))
        .RegisterCallback(gyro_cb, ahrs);

    System::Thread::Sleep(10);

    while (1) {
      ahrs->ready_.Wait(UINT32_MAX);

      if (ahrs->accl_ready_.Wait(0)) {
        accl_sub.DumpData(ahrs->accl_);
      }
      if (ahrs->gyro_ready_.Wait(0)) {
        gyro_sub.DumpData(ahrs->gyro_);
      }

      ahrs->Update();

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
static float q_2q0, q_2q1, q_2q2, q_2q3, q_4q0, q_4q1, q_4q2, q_8q1, q_8q2,
    q0q0, q1q1, q2q2, q3q3;

void AHRS::Update() {
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
