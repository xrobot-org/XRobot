/* 
	Modified from https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.h

*/

#pragma once

#include <stdint.h>

#include "user_math.h"

typedef enum {
	/* Use PID_MODE_NO_D for a PI controller (vs PID) */
	PID_MODE_NO_D = 0,
	/* PID_MODE_CALC_D calculates discrete derivative from previous error,
	 * val_dot in pid_calculate() will be ignored */
	PID_MODE_CALC_D,
	/* PID_MODE_CALC_D_NO_SP calculates discrete derivative from previous value,
	 * setpoint derivative will be ignored, val_dot in pid_calculate() will be ignored */
	PID_MODE_CALC_D_NO_SP,
	/* Use PID_MODE_SET_D if you have the derivative already (Gyros, Kalman) */
	PID_MODE_SET_D
} PID_Mode_t;

typedef struct {
	PID_Mode_t mode;
	float32_t dt_min;
	float32_t kp;
	float32_t ki;
	float32_t kd;
	float32_t i;
	float32_t i_limit;
	float32_t out_limit;
	float32_t err_last;
	float32_t out_last;
} PID_t;

typedef struct {
	float32_t kp;
	float32_t ki;
	float32_t kd;
	float32_t i_limit;
	float32_t out_limit;
} PID_Params_t;

int8_t PID_Init(PID_t *pid, PID_Mode_t mode, float32_t dt_min, const PID_Params_t *param) ;
float32_t PID_Calc(PID_t *pid, float32_t sp, float32_t val, float32_t val_dot, float32_t dt);
int8_t PID_ResetIntegral(PID_t *pid);
