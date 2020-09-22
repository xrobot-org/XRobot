/*
   Modified from
   https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.cpp
   
   参考资料：
   https://github.com/PX4/Firmware/issues/12362
   https://dev.px4.io/master/en/flight_stack/controller_diagrams.html
   https://docs.px4.io/master/en/config_mc/pid_tuning_guide_multicopter.html#standard_form
   https://www.controleng.com/articles/not-all-pid-controllers-are-the-same/
   https://en.wikipedia.org/wiki/PID_controller#Steady-state_error
*/

#include "pid.h"

#include <stdbool.h>

#define SIGMA 0.000001f

int8_t PID_Init(KPID_t *pid, KPID_Mode_t mode, float sample_freq,
                const KPID_Params_t *param) {
  if (pid == NULL) return -1;

  if (!isfinite(param->p)) return -1;
  if (!isfinite(param->i)) return -1;
  if (!isfinite(param->d)) return -1;
  if (!isfinite(param->i_limit)) return -1;
  if (!isfinite(param->out_limit)) return -1;
  pid->param = param;
  
  float dt_min = 1.0f / sample_freq;
  if (isfinite(dt_min))
    pid->dt_min = dt_min;
  else
    return -1;
  
  LowPassFilter2p_Init(&(pid->dfilter), sample_freq, pid->param->d_cutoff_freq);
  
  pid->mode = mode;
  pid->i = 0.0f;
  pid->err_last = 0.0f;
  pid->out_last = 0.0f;
  return 0;
}

float PID_Calc(KPID_t *pid, float sp, float fb, float val_dot, float dt) {
  if (!isfinite(sp) || !isfinite(fb) || !isfinite(val_dot) || !isfinite(dt)) {
    return pid->out_last;
  }

  /* current error value */
  float error = sp - fb;
  error *= pid->param->k;
  
  float val_scaled = pid->param->k * fb;
  val_scaled = LowPassFilter2p_Apply(&(pid->dfilter), val_scaled);
  
  /* current error derivative */
  float d;
  switch (pid->mode) {
    case KPID_MODE_CALC_D_ERR:
      d = (error - pid->err_last) / fmaxf(dt, pid->dt_min);
      pid->err_last = error;
      break;
    
    case KPID_MODE_CALC_D_VAL:
      d = (val_scaled + pid->err_last) / fmaxf(dt, pid->dt_min);
      pid->err_last = -val_scaled;
      break;

    case KPID_MODE_SET_D:
      d = val_dot;
      break;

    case KPID_MODE_NO_D:
      d = 0.0f;
      break;
  }

  if (!isfinite(d)) d = 0.0f;

  /* calculate PD output */
  float output = (error * pid->param->p) - (d * pid->param->d);

  if (pid->param->i > SIGMA) {
    // Calculate the error i and check for saturation
    float i = pid->i + (error * dt);

    /* check for saturation */
    if (isfinite(i)) {
      if ((pid->param->out_limit < SIGMA ||
           (fabsf(output + (i * pid->param->i)) <= pid->param->out_limit)) &&
          fabsf(i) <= pid->param->i_limit) {
        /* not saturated, use new i value */
        pid->i = i;
      }
    }

    /* add I component to output */
    output += pid->i * pid->param->i;
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

int8_t PID_ResetIntegral(KPID_t *pid) {
  if (pid == NULL) return -1;

  pid->i = 0.0f;

  return 0;
}

int8_t PID_Reset(KPID_t *pid) {
  if (pid == NULL) return -1;

  pid->i = 0.0f;
  pid->err_last = 0.0f;
  pid->out_last = 0.0f;
  LowPassFilter2p_Reset(&(pid->dfilter), 0.0f);

  return 0;
}
