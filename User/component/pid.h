/*
  Modified from
  https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.h
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "filter.h"
#include "user_math.h"

typedef enum {
  KPID_MODE_NO_D = 0, /* 不使用微分项，PI控制器 */
  KPID_MODE_CALC_D_ERR, /* 根据误差的值计算离散微分，忽略PID_Calc中的val_dot */
  KPID_MODE_CALC_D_VAL, /* 根据反馈的值计算离散微分，忽略PID_Calc中的val_dot */
  KPID_MODE_SET_D /* 直接提供微分值，PID_Calc中的val_dot将被使用，(Gyros) */
} KPID_Mode_t; /* PID模式 */

typedef struct {
  float k;           /* 控制器增益，设置为1用于并行模式 */
  float p;           /* 比例项增益，设置为1用于标准形式 */
  float i;           /* 积分项增益 */
  float d;           /* 微分项增益 */
  float i_limit;      /* 积分项上限 */
  float out_limit;    /* 输出绝对值限制 */
  float d_cutoff_freq; /* D项低通截止频率 */
} KPID_Params_t;       /* PID参数 */

typedef struct {
  KPID_Mode_t mode;
  const KPID_Params_t *param;

  float dt_min;   /* 最小PID_Calc调用间隔 */
  float i;        /* 积分 */
  float err_last; /* 上次误差 */
  float out_last; /* 上次输出 */
  LowPassFilter2p_t dfilter; /* D项低通滤波器 */
} KPID_t;          /* PID主结构体 */

int8_t PID_Init(KPID_t *pid, KPID_Mode_t mode, float sample_freq,
                const KPID_Params_t *param);
float PID_Calc(KPID_t *pid, float sp, float fb, float val_dot, float dt);
int8_t PID_ResetIntegral(KPID_t *pid);
int8_t PID_Reset(KPID_t *pid);

#ifdef __cplusplus
}
#endif
