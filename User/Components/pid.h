/* 
	Modified from https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.h

*/

#pragma once

#include <stdint.h>

typedef enum {
	/* Use PID_MODE_DERIVATIV_NONE for a PI controller (vs PID) */
	PID_MODE_DERIVATIV_NONE = 0,
	/* PID_MODE_DERIVATIV_CALC calculates discrete derivative from previous error,
	 * val_dot in pid_calculate() will be ignored */
	PID_MODE_DERIVATIV_CALC,
	/* PID_MODE_DERIVATIV_CALC_NO_SP calculates discrete derivative from previous value,
	 * setpoint derivative will be ignored, val_dot in pid_calculate() will be ignored */
	PID_MODE_DERIVATIV_CALC_NO_SP,
	/* Use PID_MODE_DERIVATIV_SET if you have the derivative already (Gyros, Kalman) */
	PID_MODE_DERIVATIV_SET
} PID_Mode_t;

typedef struct {
	PID_Mode_t mode;
	float dt_min;
	float kp;
	float ki;
	float kd;
	float integral;
	float integral_limit;
	float output_limit;
	float error_previous;
	float last_output;
} PID_t;

typedef struct {
	float kp;
	float ki;
	float kd;
	float integral_limit;
	float output_limit;
} PID_Params_t;

int PID_Init(PID_t *pid, PID_Mode_t mode, float dt_min, const PID_Params_t *param) ;
float PID_Calc(PID_t *pid, float sp, float val, float val_dot, float dt);
int PID_ResetIntegral(PID_t *pid);
