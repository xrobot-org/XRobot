/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "user_math.h"

typedef struct {
  float yaw;   /* 偏航角（Yaw angle） */
  float pit;   /* 俯仰角（Pitch angle） */
  float rol;   /* 翻滚角（Roll angle） */
} AHRS_Eulr_t; /* 欧拉角（Euler angle） */

typedef struct {
  float x;
  float y;
  float z;
} AHRS_Accl_t; /* 加速度计 Accelerometer */

typedef struct {
  float x;
  float y;
  float z;
} AHRS_Gyro_t; /* 陀螺仪 Gyroscope */

typedef struct {
  float x;
  float y;
  float z;
} AHRS_Magn_t; /* 磁力计 Magnetometer */

typedef struct {
  /* 四元数 */
  float q0;
  float q1;
  float q2;
  float q3;

  float inv_sample_freq; /* 采样频率的的倒数 */
} AHRS_t; /* 姿态解算算法主结构体 */

int8_t AHRS_Init(AHRS_t *ahrs, const AHRS_Magn_t *magn, float sample_freq);
int8_t AHRS_Update(AHRS_t *ahrs, const AHRS_Accl_t *accl,
                   const AHRS_Gyro_t *gyro, const AHRS_Magn_t *magn);
int8_t AHRS_GetEulr(AHRS_Eulr_t *eulr, const AHRS_t *ahrs);

#ifdef __cplusplus
}
#endif
