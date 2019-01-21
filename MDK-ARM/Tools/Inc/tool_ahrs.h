// Madgwick Original.
#ifndef __TOOLS_AHRS__H
#define __TOOLS_AHRS__H

#include "bsp_imu.h"

typedef struct {
	struct {
		float yaw;
		float pit;
		float rol;
	} eulr;
	
} AHRS_HandleTypeDef;

void AHRS_Init(AHRS_HandleTypeDef* hahrs, const IMU_HandleTypeDef* himu, float sample_freq);
void AHRS_Update(AHRS_HandleTypeDef* hahrs, const IMU_HandleTypeDef* himu);

#endif
