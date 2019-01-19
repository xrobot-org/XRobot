#ifndef __TOOLS_PID__H
#define __TOOLS_PID__H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	/* User parameter */
	float kp;
	float ki;
	float kd;
	float abs_limit;
	
	/* Mid product */
	float proportional;
	float integral;
	float derivative;
	
	float error;
	float last_error;
	float out;
	float cliped;
	bool use_clip;
	
} PID_HandleTypeDef;

void PID_Init(PID_HandleTypeDef *hpid, float kp, float ki, float kd, float abs_limit);
void PID_Update(PID_HandleTypeDef *hpid, float set, float get, float *p_out);

#endif
