/*
  开源的AHRS算法。
  MadgwickAHRS
*/

#pragma once

#include "comp_type.h"
#include "comp_utils.h"

/* 姿态解算算法主结构体 */
typedef struct {
  /* 四元数 */
  quaternion_t quat;

  float last_update;
  float dt;
} ahrs_t;

/**
 * @brief 初始化姿态解算
 *
 * @param ahrs 姿态解算主结构体
 * @param magn 磁力计数据
 * @return int8_t 0对应没有错误
 */
int8_t ahrs_init(ahrs_t *ahrs, const vector3_t *magn);

/**
 * @brief 姿态运算更新一次
 *
 * @param ahrs 姿态解算主结构体
 * @param accl 加速度计数据
 * @param gyro 陀螺仪数据
 * @param magn 磁力计数据
 * @param now 现在时刻
 * @return int8_t 0对应没有错误
 */
int8_t ahrs_update(ahrs_t *ahrs, const vector3_t *accl, const vector3_t *gyro,
                   const vector3_t *magn, float now);

/**
 * @brief 通过姿态解算主结构体中的四元数计算欧拉角
 *
 * @param eulr 欧拉角
 * @param ahrs 姿态解算主结构体
 * @return int8_t 0对应没有错误
 */
int8_t ahrs_get_eulr(eulr_t *eulr, const ahrs_t *ahrs);

/**
 * @brief 将对应数据置零
 *
 * @param eulr 被操作的数据
 */
void ahrs_reset_eulr(eulr_t *eulr);
