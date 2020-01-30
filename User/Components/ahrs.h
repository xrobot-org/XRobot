/* 
	开源的AHRS算法。

*/

#pragma once

#include "imu.h"

typedef struct {
	struct {
		float yaw;
		float pit;
		float rol;
	} eulr;

	float q0;
	float q1;
	float q2;
	float q3;

	float rot_matrix[3][3];

	float inv_sample_freq;
	
} AHRS_t;

int AHRS_Init(AHRS_t *ahrs, const IMU_t *imu, float sample_freq);
int AHRS_Update(AHRS_t *ahrs, const IMU_t *imu);
