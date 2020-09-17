/*
  Modified from
  https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.h
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "user_math.h"

typedef enum {
  PID_MODE_NO_D = 0, /* 不使用微分项，PI控制器 */
  PID_MODE_CALC_D, /* 根据先前的误差计算离散微分，忽略PID_Calc中的val_dot */
  PID_MODE_CALC_D_NO_SP, /* 根据先前的值计算离散微分，忽略PID_Calc中的val_dot */
  PID_MODE_SET_D /* 直接提供微分值，PID_Calc中的val_dot将被使用，(Gyros) */
} PID_Mode_t; /* PID模式 */

typedef struct {
  float kp;        /* 比例项参数 */
  float ki;        /* 积分项参数 */
  float kd;        /* 微分项参数 */
  float i_limit;   /* 积分项上线 */
  float out_limit; /* 输出绝对值限制 */
} PID_Params_t;    /* PID参数 */

typedef struct {
  PID_Mode_t mode;
  const PID_Params_t *param;

  float dt_min;   /* 最小PID_Calc调用间隔 */
  float i;        /* 积分 */
  float err_last; /* 上次误差 */
  float out_last; /* 上次输出 */
} PID_t;          /* PID主结构体 */

int8_t PID_Init(PID_t *pid, PID_Mode_t mode, float dt_min,
                const PID_Params_t *param);
float PID_Calc(PID_t *pid, float sp, float val, float val_dot, float dt);
int8_t PID_ResetIntegral(PID_t *pid);

#ifdef __cplusplus
}
#endif
