/* 
	开源的AHRS算法。
	MadgwickAHRS
*/

#pragma once

#include "user_math.h"

typedef struct {
	float32_t yaw;
	float32_t pit;
	float32_t rol;
} AHRS_Eulr_t;

typedef	struct {
	float32_t x;
	float32_t y;
	float32_t z;
} AHRS_Accl_t;
	
typedef	struct {
	float32_t x;
	float32_t y;
	float32_t z;
} AHRS_Gyro_t;

typedef	struct {
	float32_t x;
	float32_t y;
	float32_t z;
} AHRS_Magn_t;

typedef struct {
	float32_t q0;
	float32_t q1;
	float32_t q2;
	float32_t q3;

	float32_t inv_sample_freq;
} AHRS_t;

int8_t AHRS_Init(AHRS_t *ahrs, const AHRS_Magn_t *magn, float32_t sample_freq);
int8_t AHRS_Update(AHRS_t *ahrs, const AHRS_Accl_t *accl, const AHRS_Gyro_t *gyro, const AHRS_Magn_t *magn);
int8_t AHRS_GetEulr(AHRS_Eulr_t *eulr, const AHRS_t *ahrs);
