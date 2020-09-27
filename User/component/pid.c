/*
   Modified from
   https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.cpp

   参考资料：
   https://github.com/PX4/Firmware/issues/12362
   https://dev.px4.io/master/en/flight_stack/controller_diagrams.html
   https://docs.px4.io/master/en/config_mc/pid_tuning_guide_multicopter.html#standard_form
   https://www.controleng.com/articles/not-all-pid-controllers-are-the-same/
   https://en.wikipedia.org/wiki/PID_controller
   http://brettbeauregard.com/blog/2011/04/improving-the-beginner%E2%80%99s-pid-derivative-kick/
*/

#include "pid.h"

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
  pid->last.err = 0.0f;
  pid->out_last = 0.0f;
  return 0;
}

float PID_Calc(KPID_t *pid, float sp, float fb, float val_dot, float dt) {
  if (!isfinite(sp) || !isfinite(fb) || !isfinite(val_dot) || !isfinite(dt)) {
    return pid->out_last;
  }

  /* 计算误差值 */
  float err;
  if (pid->param->range > 0.0f) {
    err = CalcCircleError(sp, fb, pid->param->range);
  } else {
    err = sp - fb;
  }

  err *= pid->param->k;

  float fb_scaled = pid->param->k * fb;
  fb_scaled = LowPassFilter2p_Apply(&(pid->dfilter), fb_scaled);

  /* 现在的误差微分 */
  float d;
  switch (pid->mode) {
    case KPID_MODE_CALC_D_ERR:
      d = (err - pid->last.err) / fmaxf(dt, pid->dt_min);
      pid->last.err = err;
      break;

    /* 通过fb计算D，避免了由于sp变化导致err突变的问题 */
    /* 当sp不变时，err的微分等于负的fb的微分 */
    case KPID_MODE_CALC_D_FB:
      d = -(fb_scaled - pid->last.fb) / fmaxf(dt, pid->dt_min);
      pid->last.fb = fb_scaled;
      break;

    case KPID_MODE_SET_D:
      d = -val_dot;
      break;

    case KPID_MODE_NO_D:
      d = 0.0f;
      break;
  }

  if (!isfinite(d)) d = 0.0f;

  /* 计算P和D输出 */
  float output = (err * pid->param->p) + (d * pid->param->d);

  if (pid->param->i > SIGMA) {
    /* 计算误差的积分 */
    float i = pid->i + (err * dt);

    /* 检查是否饱和 */
    if (isfinite(i)) {
      if ((pid->param->out_limit < SIGMA ||
           (fabsf(output + (i * pid->param->i)) <= pid->param->out_limit)) &&
          fabsf(i) <= pid->param->i_limit) {
        /* 未饱和，使用新积分 */
        pid->i = i;
      }
    }

    /* 给输出添加积分项 */
    output += pid->i * pid->param->i;
  }

  /* 限制输出 */
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
  pid->last.err = 0.0f;
  pid->out_last = 0.0f;
  LowPassFilter2p_Reset(&(pid->dfilter), 0.0f);

  return 0;
}
