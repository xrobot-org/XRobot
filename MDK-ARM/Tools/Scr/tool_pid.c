#include "tool_pid.h"

#include <math.h>

static float AbsClip(float in, float limit) {
	return (in < -limit) ? -limit : ((in > limit) ? limit : in);
}

static float Sign(float in) {
	return (in > 0) ? 1.f : 0.f;
}

/* Set abs_limit little smaller than real world limitation in some cases. */
void PID_Init(PID_HandleTypeDef* hpid, float kp, float ki, float kd, float abs_limit) {
	hpid->kp = kp;
	hpid->ki = ki;
	hpid->kd = kd;
	hpid->abs_limit = abs_limit;
}

void PID_Update(PID_HandleTypeDef* hpid, float set, float get, float* p_out) {
	hpid->error = set - get;
	
	/* Present */
	hpid->proportional = hpid->kp * hpid->error;
	
	/* Post */
	hpid->integral += (hpid->use_clip) ? 0.f : hpid->ki * hpid->error;
	
	/* Future */
	hpid->derivative = hpid->kd * (hpid->error - hpid->last_error);
	hpid->last_error = hpid->error;
	
	hpid->out = hpid->proportional + hpid->integral + hpid->derivative;
	
	/* Anti-windup (Clamping) */
	hpid->cliped = AbsClip(hpid->out, hpid->abs_limit);

	if ((hpid->cliped != hpid->out) && (Sign(hpid->error) == Sign(hpid->out)))
		hpid->use_clip = true;
	
	*p_out = hpid->cliped;
}
