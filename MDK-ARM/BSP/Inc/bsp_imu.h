#ifndef __BSP_IMU__H
#define __BSP_IMU__H

#include "bsp_common.h"

typedef struct {
	struct {
		float x;
		float y;
		float z;
	} accl;
	
	struct {
		float x;
		float y;
		float z;
	} gyro;
	
	struct {
		float x;
		float y;
		float z;
	} magn;
	
	float temp;
} IMU_HandleTypeDef;

BSP_StatusTypedef IMU_Init(void);
BSP_StatusTypedef IMU_Update(IMU_HandleTypeDef *himu);
#endif
