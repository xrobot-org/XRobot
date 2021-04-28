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

/**
 * @brief 初始化PID
 *
 * @param pid PID结构体
 * @param mode PID模式
 * @param sample_freq 采样频率
 * @param param PID参数
 * @return int8_t 0对应没有错误
 */
int8_t PID_Init(KPID_t *pid, KPID_Mode_t mode, float sample_freq,
                const KPID_Params_t *param) {
  ASSERT(pid);

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
  PID_Reset(pid);
  return 0;
}

/**
 * @brief PID计算
 *
 * @param pid PID结构体
 * @param sp 设定值
 * @param fb 反馈值
 * @param fb_dot 反馈值微分
 * @param dt 间隔时间
 * @return float 计算的输出
 */
float PID_Calc(KPID_t *pid, float sp, float fb, float fb_dot, float dt) {
  if (!isfinite(sp) || !isfinite(fb) || !isfinite(fb_dot) || !isfinite(dt)) {
    return pid->last.out;
  }

  /* 计算误差值 */
  const float err = CircleError(sp, fb, pid->param->range);

  /* 计算P项 */
  const float k_err = err * pid->param->k;

  /* 计算D项 */
  const float k_fb = pid->param->k * fb;
  const float filtered_k_fb = LowPassFilter2p_Apply(&(pid->dfilter), k_fb);

  float d;
  switch (pid->mode) {
    case KPID_MODE_CALC_D:
      /* 通过fb计算D，避免了由于sp变化导致err突变的问题 */
      /* 当sp不变时，err的微分等于负的fb的微分 */
      d = (filtered_k_fb - pid->last.k_fb) / fmaxf(dt, pid->dt_min);
      break;

    case KPID_MODE_SET_D:
      d = fb_dot;
      break;

    case KPID_MODE_NO_D:
      d = 0.0f;
      break;
  }
  pid->last.err = err;
  pid->last.k_fb = filtered_k_fb;

  if (!isfinite(d)) d = 0.0f;

  /* 计算PD输出 */
  float output = (k_err * pid->param->p) - (d * pid->param->d);

  /* 计算I项 */
  const float i = pid->i + (k_err * dt);
  const float i_out = i * pid->param->i;

  if (pid->param->i > SIGMA) {
    /* 检查是否饱和 */
    if (isfinite(i)) {
      if ((fabsf(output + i_out) <= pid->param->out_limit) &&
          (fabsf(i) <= pid->param->i_limit)) {
        /* 未饱和，使用新积分 */
        pid->i = i;
      }
    }
  }

  /* 计算PID输出 */
  output += i_out;

  /* 限制输出 */
  if (isfinite(output)) {
    if (pid->param->out_limit > SIGMA) {
      output = AbsClamp(output, pid->param->out_limit);
    }
    pid->last.out = output;
  }
  return pid->last.out;
}

/**
 * @brief 重置微分项
 *
 * @param pid PID结构体
 * @return int8_t 0对应没有错误
 */
int8_t PID_ResetIntegral(KPID_t *pid) {
  ASSERT(pid);

  pid->i = 0.0f;

  return 0;
}

/**
 * @brief 重置PID
 *
 * @param pid PID结构体
 * @return int8_t 0对应没有错误
 */
int8_t PID_Reset(KPID_t *pid) {
  ASSERT(pid);

  pid->i = 0.0f;
  pid->last.err = 0.0f;
  pid->last.k_fb = 0.0f;
  pid->last.out = 0.0f;
  LowPassFilter2p_Reset(&(pid->dfilter), 0.0f);

  return 0;
}
