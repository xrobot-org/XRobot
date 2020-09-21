/*
   Modified from
   https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.cpp
*/

#include "pid.h"

#include <stdbool.h>

#define SIGMA 0.000001f

int8_t PID_Init(PID_t *pid, PID_Mode_t mode, float dt_min,
                const PID_Params_t *param) {
  if (pid == NULL) return -1;

  if (!isfinite(param->kp)) return -1;
  if (!isfinite(param->ki)) return -1;
  if (!isfinite(param->kd)) return -1;
  if (!isfinite(param->i_limit)) return -1;
  if (!isfinite(param->out_limit)) return -1;
  pid->param = param;

  if (isfinite(dt_min))
    pid->dt_min = dt_min;
  else
    return -1;

  pid->mode = mode;
  pid->i = 0.0f;
  pid->err_last = 0.0f;
  pid->out_last = 0.0f;
  return 0;
}

float PID_Calc(PID_t *pid, float sp, float val, float val_dot, float dt) {
  if (!isfinite(sp) || !isfinite(val) || !isfinite(val_dot) || !isfinite(dt)) {
    return pid->out_last;
  }

  float i, d;

  /* current error value */
  float error = sp - val;

  /* current error derivative */
  switch (pid->mode) {
    case PID_MODE_CALC_D:
      d = (error - pid->err_last) / fmaxf(dt, pid->dt_min);
      pid->err_last = error;
      break;

    case PID_MODE_CALC_D_NO_SP:
      d = (-val - pid->err_last) / fmaxf(dt, pid->dt_min);
      pid->err_last = -val;
      break;

    case PID_MODE_SET_D:
      d = val_dot;
      break;

    case PID_MODE_NO_D:
      d = 0.0f;
      break;
  }

  if (!isfinite(d)) d = 0.0f;

  /* calculate PD output */
  float output = (error * pid->param->kp) - (d * pid->param->kd);

  if (pid->param->ki > SIGMA) {
    // Calculate the error i and check for saturation
    i = pid->i + (error * dt);

    /* check for saturation */
    if (isfinite(i)) {
      if ((pid->param->out_limit < SIGMA ||
           (fabsf(output + (i * pid->param->ki)) <= pid->param->out_limit)) &&
          fabsf(i) <= pid->param->i_limit) {
        /* not saturated, use new i value */
        pid->i = i;
      }
    }

    /* add I component to output */
    output += pid->i * pid->param->ki;
  }

  /* limit output */
  if (isfinite(output)) {
    if (pid->param->out_limit > SIGMA) {
      output = AbsClip(output, pid->param->out_limit);
    }
    pid->out_last = output;
  }

  return pid->out_last;
}

int8_t PID_ResetIntegral(PID_t *pid) {
  if (pid == NULL) return -1;

  pid->i = 0.0f;

  return 0;
}

int8_t PID_Reset(PID_t *pid) {
  if (pid == NULL) return -1;

  pid->i = 0.0f;
  pid->err_last = 0.0f;
  pid->out_last = 0.0f;

  return 0;
}
