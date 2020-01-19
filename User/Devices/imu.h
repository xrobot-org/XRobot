#pragma once

#include "board.h"

typedef struct {
	struct {
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
	} data;
	
	struct {
		float gyro_offset[3];
		float magn_offset[3];
		float magn_scale[3];
		float temp;
	} cali;
	
} IMU_t;

int IMU_CaliGyro(IMU_t* himu);
int IMU_Init(IMU_t* himu);
int IMU_Update(IMU_t* himu);
