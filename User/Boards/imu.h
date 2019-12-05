#pragma once

#include "board.h"

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

Board_Status_t IMU_Init(void);
Board_Status_t IMU_Update(IMU_HandleTypeDef* himu);
