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
		struct {
			float x;
			float y;
			float z;
		} gyro_offset;
		
		struct {
			float x;
			float y;
			float z;
		} magn_offset;
		
		struct {
			float x;
			float y;
			float z;
		} magn_scale;
		
		float temp;
	} cali;
	
} IMU_t;

Board_Status_t IMU_Init(void);
Board_Status_t IMU_Update(IMU_t* himu);
