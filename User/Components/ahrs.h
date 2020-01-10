/* 
	开源的AHRS算法。
	
	考虑使用Botch的BSXlite 

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

void AHRS_Init(AHRS_t* hahrs, const IMU_t* himu, float sample_freq);
void AHRS_Update(AHRS_t* hahrs, const IMU_t* himu);
