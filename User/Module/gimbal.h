#pragma once

typedef enum {
	GIMBAL_MODE_RELAX,
	GIMBAL_MODE_INIT,
	GIMBAL_MODE_CALI,
	GIMBAL_MODE_ABSOLUTE, 
  GIMBAL_MODE_RELATIVE, 
	GIMBAL_MODE_FIX,
} Gimbal_Mode_t;

typedef struct {
	float motor;
	int mode ;
	int mixer;
	
} Gimbal_t;

void Gimbal_Init(void);

