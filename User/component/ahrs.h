/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#pragma once

#include "utils.h"

/* 欧拉角（Euler angle） */
typedef struct {
  float yaw; /* 偏航角（Yaw angle） */
  float pit; /* 俯仰角（Pitch angle） */
  float rol; /* 翻滚角（Roll angle） */
} AHRS_Eulr_t;

/* 加速度计 Accelerometer */
typedef struct {
  float x;
  float y;
  float z;
} AHRS_Accl_t;

/* 陀螺仪 Gyroscope */
typedef struct {
  float x;
  float y;
  float z;
} AHRS_Gyro_t;

/* 磁力计 Magnetometer */
typedef struct {
  float x;
  float y;
  float z;
} AHRS_Magn_t;

/* 四元数 */
typedef struct {
  float q0;
  float q1;
  float q2;
  float q3;
} AHRS_Quaternion_t;

/* 姿态解算算法主结构体 */
typedef struct {
  /* 四元数 */
  AHRS_Quaternion_t quat;

  float inv_sample_freq; /* 采样频率的的倒数 */
} AHRS_t;

/**
 * @brief 初始化姿态解算
 *
 * @param ahrs 姿态解算主结构体
 * @param magn 磁力计数据
 * @param sample_freq 采样频率
 * @return int8_t 0对应没有错误
 */
int8_t AHRS_Init(AHRS_t *ahrs, const AHRS_Magn_t *magn, float sample_freq);

/**
 * @brief 姿态运算更新一次
 *
 * @param ahrs 姿态解算主结构体
 * @param accl 加速度计数据
 * @param gyro 陀螺仪数据
 * @param magn 磁力计数据
 * @return int8_t 0对应没有错误
 */
int8_t AHRS_Update(AHRS_t *ahrs, const AHRS_Accl_t *accl,
                   const AHRS_Gyro_t *gyro, const AHRS_Magn_t *magn);

/**
 * @brief 通过姿态解算主结构体中的四元数计算欧拉角
 *
 * @param eulr 欧拉角
 * @param ahrs 姿态解算主结构体
 * @return int8_t 0对应没有错误
 */
int8_t AHRS_GetEulr(AHRS_Eulr_t *eulr, const AHRS_t *ahrs);

/**
 * \brief 将对应数据置零
 *
 * \param eulr 被操作的数据
 */
void AHRS_ResetEulr(AHRS_Eulr_t *eulr);
