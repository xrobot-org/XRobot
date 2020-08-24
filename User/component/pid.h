/* 
	Modified from https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.h

*/

#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

#include "user_math.h"

typedef enum {
	/* Use PID_MODE_NO_D for a M_PI controller (vs PID) */
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
	float dt_min;
	float kp;
	float ki;
	float kd;
	float i;
	float i_limit;
	float out_limit;
	float err_last;
	float out_last;
} PID_t;

typedef struct {
	float kp;
	float ki;
	float kd;
	float i_limit;
	float out_limit;
} PID_Params_t;

int8_t PID_Init(PID_t *pid, PID_Mode_t mode, float dt_min, const PID_Params_t *param) ;
float PID_Calc(PID_t *pid, float sp, float val, float val_dot, float dt);
int8_t PID_ResetIntegral(PID_t *pid);

#ifdef __cplusplus
}
#endif
