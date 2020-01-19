/* 
	Modified from https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.cpp

*/


#include "pid.h"

#include "user_math.h"

#define SIGMA 0.000001f

void PID_Init(PID_t *pid, PID_Mode_t mode, float dt_min) {
	pid->mode = mode;
	pid->dt_min = dt_min;
	pid->kp = 0.0f;
	pid->ki = 0.0f;
	pid->kd = 0.0f;
	pid->integral = 0.0f;
	pid->integral_limit = 0.0f;
	pid->output_limit = 0.0f;
	pid->error_previous = 0.0f;
	pid->last_output = 0.0f;
}


int PID_SetParameters(PID_t *pid, float kp, float ki, float kd, float integral_limit, float output_limit) {
	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;
	pid->integral_limit = integral_limit;
	pid->output_limit = output_limit;

	return 0;
}

float PID_Calculate(PID_t *pid, float sp, float val, float val_dot, float dt)
{
	float i, d;

	/* current error value */
	float error = sp - val;

	/* current error derivative */
	if (pid->mode == PID_MODE_DERIVATIV_CALC) {
		d = (error - pid->error_previous) / fmaxf(dt, pid->dt_min);
		pid->error_previous = error;

	} else if (pid->mode == PID_MODE_DERIVATIV_CALC_NO_SP) {
		d = (-val - pid->error_previous) / fmaxf(dt, pid->dt_min);
		pid->error_previous = -val;

	} else if (pid->mode == PID_MODE_DERIVATIV_SET) {
		d = -val_dot;

	} else {
		d = 0.0f;
	}

	/* calculate PD output */
	float output = (error * pid->kp) + (d * pid->kd);

	if (pid->ki > SIGMA) {
		// Calculate the error integral and check for saturation
		i = pid->integral + (error * dt);

		/* check for saturation */
		if ((pid->output_limit < SIGMA || (fabsf(output + (i * pid->ki)) <= pid->output_limit)) &&
			fabsf(i) <= pid->integral_limit) {
			/* not saturated, use new integral value */
			pid->integral = i;
		}

		/* add I component to output */
		output += pid->integral * pid->ki;
	}

	/* limit output */
	if (pid->output_limit > SIGMA) {
		if (output > pid->output_limit) {
			output = pid->output_limit;

		} else if (output < -pid->output_limit) {
			output = -pid->output_limit;
		}
	}

	pid->last_output = output;


	return pid->last_output;
}

void PID_ResetIntegral(PID_t *pid)
{
	pid->integral = 0.0f;
}
