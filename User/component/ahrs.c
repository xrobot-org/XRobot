/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#include "ahrs.h"

#include <string.h>

#include "user_math.h"

#define BETA_IMU (0.033f)
#define BETA_AHRS (0.041f)

/* 2 * proportional gain (Kp) */
static float beta = BETA_IMU;

static int8_t AHRS_UpdateIMU(AHRS_t *ahrs, const AHRS_Accl_t *accl,
                             const AHRS_Gyro_t *gyro) {
  if (ahrs == NULL) return -1;
  if (accl == NULL) return -1;
  if (gyro == NULL) return -1;

  beta = BETA_IMU;

  float ax = accl->x;
  float ay = accl->y;
  float az = accl->z;

  float gx = gyro->x;
  float gy = gyro->y;
  float gz = gyro->z;

  float recip_norm;
  float s0, s1, s2, s3;
  float q_dot1, q_dot2, q_dot3, q_dot4;
  float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1, q2q2,
      q3q3;

  // Rate of change of quaternion from gyroscope
  q_dot1 = 0.5f * (-ahrs->q1 * gx - ahrs->q2 * gy - ahrs->q3 * gz);
  q_dot2 = 0.5f * (ahrs->q0 * gx + ahrs->q2 * gz - ahrs->q3 * gy);
  q_dot3 = 0.5f * (ahrs->q0 * gy - ahrs->q1 * gz + ahrs->q3 * gx);
  q_dot4 = 0.5f * (ahrs->q0 * gz + ahrs->q1 * gy - ahrs->q2 * gx);

  // Compute feedback only if accelerometer measurement valid (avoids NaN in
  // accelerometer normalisation)
  if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
    // Normalise accelerometer measurement
    recip_norm = InvSqrt(ax * ax + ay * ay + az * az);
    ax *= recip_norm;
    ay *= recip_norm;
    az *= recip_norm;

    // Auxiliary variables to avoid repeated arithmetic
    _2q0 = 2.0f * ahrs->q0;
    _2q1 = 2.0f * ahrs->q1;
    _2q2 = 2.0f * ahrs->q2;
    _2q3 = 2.0f * ahrs->q3;
    _4q0 = 4.0f * ahrs->q0;
    _4q1 = 4.0f * ahrs->q1;
    _4q2 = 4.0f * ahrs->q2;
    _8q1 = 8.0f * ahrs->q1;
    _8q2 = 8.0f * ahrs->q2;
    q0q0 = ahrs->q0 * ahrs->q0;
    q1q1 = ahrs->q1 * ahrs->q1;
    q2q2 = ahrs->q2 * ahrs->q2;
    q3q3 = ahrs->q3 * ahrs->q3;

    // Gradient decent algorithm corrective step
    s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
    s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * ahrs->q1 - _2q0 * ay - _4q1 +
         _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
    s2 = 4.0f * q0q0 * ahrs->q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 +
         _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
    s3 =
        4.0f * q1q1 * ahrs->q3 - _2q1 * ax + 4.0f * q2q2 * ahrs->q3 - _2q2 * ay;
    recip_norm = InvSqrt(s0 * s0 + s1 * s1 + s2 * s2 +
                         s3 * s3);  // normalise step magnitude
    s0 *= recip_norm;
    s1 *= recip_norm;
    s2 *= recip_norm;
    s3 *= recip_norm;

    // Apply feedback step
    q_dot1 -= beta * s0;
    q_dot2 -= beta * s1;
    q_dot3 -= beta * s2;
    q_dot4 -= beta * s3;
  }

  // Integrate rate of change of quaternion to yield quaternion
  ahrs->q0 += q_dot1 * ahrs->inv_sample_freq;
  ahrs->q1 += q_dot2 * ahrs->inv_sample_freq;
  ahrs->q2 += q_dot3 * ahrs->inv_sample_freq;
  ahrs->q3 += q_dot4 * ahrs->inv_sample_freq;

  // Normalise quaternion
  recip_norm = InvSqrt(ahrs->q0 * ahrs->q0 + ahrs->q1 * ahrs->q1 +
                       ahrs->q2 * ahrs->q2 + ahrs->q3 * ahrs->q3);
  ahrs->q0 *= recip_norm;
  ahrs->q1 *= recip_norm;
  ahrs->q2 *= recip_norm;
  ahrs->q3 *= recip_norm;

  return 0;
}

int8_t AHRS_Init(AHRS_t *ahrs, const AHRS_Magn_t *magn, float sample_freq) {
  if (ahrs == NULL) return -1;

  ahrs->inv_sample_freq = 1.0f / sample_freq;

  ahrs->q0 = 1.0f;
  ahrs->q1 = 0.0f;
  ahrs->q2 = 0.0f;
  ahrs->q3 = 0.0f;

  if (magn) {
    float yaw = -atan2(magn->y, magn->x);

    if ((magn->x == 0.0f) && (magn->y == 0.0f) && (magn->z == 0.0f)) {
      ahrs->q0 = 0.800884545f;
      ahrs->q1 = 0.00862364192f;
      ahrs->q2 = -0.00283267116f;
      ahrs->q3 = 0.598749936f;

    } else if ((yaw < (M_PI / 2)) || (yaw > 0.0f)) {
      ahrs->q0 = 0.997458339f;
      ahrs->q1 = 0.000336312107f;
      ahrs->q2 = -0.0057230792f;
      ahrs->q3 = 0.0740156546;

    } else if ((yaw < M_PI) || (yaw > (M_PI / 2))) {
      ahrs->q0 = 0.800884545f;
      ahrs->q1 = 0.00862364192f;
      ahrs->q2 = -0.00283267116f;
      ahrs->q3 = 0.598749936f;

    } else if ((yaw < 90.0f) || (yaw > M_PI)) {
      ahrs->q0 = 0.800884545f;
      ahrs->q1 = 0.00862364192f;
      ahrs->q2 = -0.00283267116f;
      ahrs->q3 = 0.598749936f;

    } else if ((yaw < 90.0f) || (yaw > 0.0f)) {
      ahrs->q0 = 0.800884545f;
      ahrs->q1 = 0.00862364192f;
      ahrs->q2 = -0.00283267116f;
      ahrs->q3 = 0.598749936f;
    }
  }
  return 0;
}

/* Feed the sensor data in NED(North East Down) reference frame. Rotation can be
 * added. */
int8_t AHRS_Update(AHRS_t *ahrs, const AHRS_Accl_t *accl,
                   const AHRS_Gyro_t *gyro, const AHRS_Magn_t *magn) {
  if (ahrs == NULL) return -1;
  if (accl == NULL) return -1;
  if (gyro == NULL) return -1;

  beta = BETA_AHRS;

  float recip_norm;
  float s0, s1, s2, s3;
  float q_dot1, q_dot2, q_dot3, q_dot4;
  float hx, hy;
  float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1,
      _2q2, _2q3, _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3,
      q2q2, q2q3, q3q3;

  if (magn == NULL) return AHRS_UpdateIMU(ahrs, accl, gyro);

  float mx = magn->x;
  float my = magn->y;
  float mz = magn->z;

  // Use IMU algorithm if magnetometer measurement invalid (avoids NaN in
  // magnetometer normalisation)
  if ((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f)) {
    return AHRS_UpdateIMU(ahrs, accl, gyro);
  }

  float ax = accl->x;
  float ay = accl->y;
  float az = accl->z;

  float gx = gyro->x;
  float gy = gyro->y;
  float gz = gyro->z;

  // Rate of change of quaternion from gyroscope
  q_dot1 = 0.5f * (-ahrs->q1 * gx - ahrs->q2 * gy - ahrs->q3 * gz);
  q_dot2 = 0.5f * (ahrs->q0 * gx + ahrs->q2 * gz - ahrs->q3 * gy);
  q_dot3 = 0.5f * (ahrs->q0 * gy - ahrs->q1 * gz + ahrs->q3 * gx);
  q_dot4 = 0.5f * (ahrs->q0 * gz + ahrs->q1 * gy - ahrs->q2 * gx);

  // Compute feedback only if accelerometer measurement valid (avoids NaN in
  // accelerometer normalisation)
  if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
    // Normalise accelerometer measurement
    recip_norm = InvSqrt(ax * ax + ay * ay + az * az);
    ax *= recip_norm;
    ay *= recip_norm;
    az *= recip_norm;

    // Normalise magnetometer measurement
    recip_norm = InvSqrt(mx * mx + my * my + mz * mz);
    mx *= recip_norm;
    my *= recip_norm;
    mz *= recip_norm;

    // Auxiliary variables to avoid repeated arithmetic
    _2q0mx = 2.0f * ahrs->q0 * mx;
    _2q0my = 2.0f * ahrs->q0 * my;
    _2q0mz = 2.0f * ahrs->q0 * mz;
    _2q1mx = 2.0f * ahrs->q1 * mx;
    _2q0 = 2.0f * ahrs->q0;
    _2q1 = 2.0f * ahrs->q1;
    _2q2 = 2.0f * ahrs->q2;
    _2q3 = 2.0f * ahrs->q3;
    _2q0q2 = 2.0f * ahrs->q0 * ahrs->q2;
    _2q2q3 = 2.0f * ahrs->q2 * ahrs->q3;
    q0q0 = ahrs->q0 * ahrs->q0;
    q0q1 = ahrs->q0 * ahrs->q1;
    q0q2 = ahrs->q0 * ahrs->q2;
    q0q3 = ahrs->q0 * ahrs->q3;
    q1q1 = ahrs->q1 * ahrs->q1;
    q1q2 = ahrs->q1 * ahrs->q2;
    q1q3 = ahrs->q1 * ahrs->q3;
    q2q2 = ahrs->q2 * ahrs->q2;
    q2q3 = ahrs->q2 * ahrs->q3;
    q3q3 = ahrs->q3 * ahrs->q3;

    // Reference direction of Earth's magnetic field
    hx = mx * q0q0 - _2q0my * ahrs->q3 + _2q0mz * ahrs->q2 + mx * q1q1 +
         _2q1 * my * ahrs->q2 + _2q1 * mz * ahrs->q3 - mx * q2q2 - mx * q3q3;
    hy = _2q0mx * ahrs->q3 + my * q0q0 - _2q0mz * ahrs->q1 + _2q1mx * ahrs->q2 -
         my * q1q1 + my * q2q2 + _2q2 * mz * ahrs->q3 - my * q3q3;
    _2bx = sqrtf(hx * hx + hy * hy);
    _2bz = -_2q0mx * ahrs->q2 + _2q0my * ahrs->q1 + mz * q0q0 +
           _2q1mx * ahrs->q3 - mz * q1q1 + _2q2 * my * ahrs->q3 - mz * q2q2 +
           mz * q3q3;
    _4bx = 2.0f * _2bx;
    _4bz = 2.0f * _2bz;

    // Gradient decent algorithm corrective step
    s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q1 * (2.0f * q0q1 + _2q2q3 - ay) -
         _2bz * ahrs->q2 *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (-_2bx * ahrs->q3 + _2bz * ahrs->q1) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         _2bx * ahrs->q2 *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q0 * (2.0f * q0q1 + _2q2q3 - ay) -
         4.0f * ahrs->q1 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) +
         _2bz * ahrs->q3 *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (_2bx * ahrs->q2 + _2bz * ahrs->q0) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         (_2bx * ahrs->q3 - _4bz * ahrs->q1) *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q3 * (2.0f * q0q1 + _2q2q3 - ay) -
         4.0f * ahrs->q2 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) +
         (-_4bx * ahrs->q2 - _2bz * ahrs->q0) *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (_2bx * ahrs->q1 + _2bz * ahrs->q3) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         (_2bx * ahrs->q0 - _4bz * ahrs->q2) *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q2 * (2.0f * q0q1 + _2q2q3 - ay) +
         (-_4bx * ahrs->q3 + _2bz * ahrs->q1) *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (-_2bx * ahrs->q0 + _2bz * ahrs->q2) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         _2bx * ahrs->q1 *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    recip_norm = InvSqrt(s0 * s0 + s1 * s1 + s2 * s2 +
                         s3 * s3);  // normalise step magnitude
    s0 *= recip_norm;
    s1 *= recip_norm;
    s2 *= recip_norm;
    s3 *= recip_norm;

    // Apply feedback step
    q_dot1 -= beta * s0;
    q_dot2 -= beta * s1;
    q_dot3 -= beta * s2;
    q_dot4 -= beta * s3;
  }

  // Integrate rate of change of quaternion to yield quaternion
  ahrs->q0 += q_dot1 * ahrs->inv_sample_freq;
  ahrs->q1 += q_dot2 * ahrs->inv_sample_freq;
  ahrs->q2 += q_dot3 * ahrs->inv_sample_freq;
  ahrs->q3 += q_dot4 * ahrs->inv_sample_freq;

  // Normalise quaternion
  recip_norm = InvSqrt(ahrs->q0 * ahrs->q0 + ahrs->q1 * ahrs->q1 +
                       ahrs->q2 * ahrs->q2 + ahrs->q3 * ahrs->q3);
  ahrs->q0 *= recip_norm;
  ahrs->q1 *= recip_norm;
  ahrs->q2 *= recip_norm;
  ahrs->q3 *= recip_norm;

  return 0;
}

int8_t AHRS_GetEulr(AHRS_Eulr_t *eulr, const AHRS_t *ahrs) {
  if (eulr == NULL) return -1;
  if (ahrs == NULL) return -1;

  const float sinr_cosp = 2.0f * (ahrs->q0 * ahrs->q1 + ahrs->q2 * ahrs->q3);
  const float cosr_cosp =
      1.0f - 2.0f * (ahrs->q1 * ahrs->q1 + ahrs->q2 * ahrs->q2);
  eulr->pit = atan2f(sinr_cosp, cosr_cosp);

  const float sinp = 2.0f * (ahrs->q0 * ahrs->q2 - ahrs->q3 * ahrs->q1);

  if (fabsf(sinp) >= 1.0f)
    eulr->rol = copysignf(M_PI / 2.0f, sinp);
  else
    eulr->rol = asinf(sinp);

  const float siny_cosp = 2.0f * (ahrs->q0 * ahrs->q3 + ahrs->q1 * ahrs->q2);
  const float cosy_cosp =
      1.0f - 2.0f * (ahrs->q2 * ahrs->q2 + ahrs->q3 * ahrs->q3);
  eulr->yaw = atan2f(siny_cosp, cosy_cosp);

#if 1
  eulr->yaw *= MATH_RAD_TO_DEG_MULT;
  eulr->rol *= MATH_RAD_TO_DEG_MULT;
  eulr->pit *= MATH_RAD_TO_DEG_MULT;
#endif

  return 0;
}

void AHRS_ResetEulr(AHRS_Eulr_t *eulr) { memset(eulr, 0, sizeof(AHRS_Eulr_t)); }

void AHRS_ResetAccl(AHRS_Accl_t *accl) { memset(accl, 0, sizeof(AHRS_Accl_t)); }

void AHRS_ResetGyro(AHRS_Gyro_t *gyro) { memset(gyro, 0, sizeof(AHRS_Gyro_t)); }

void AHRS_ResetMagn(AHRS_Magn_t *magn) { memset(magn, 0, sizeof(AHRS_Magn_t)); }
