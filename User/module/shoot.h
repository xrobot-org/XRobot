/*
  射击模组
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <cmsis_os2.h>

#include "component\cmd.h"
#include "component\filter.h"
#include "component\pid.h"
#include "device\can.h"

/* Exported constants --------------------------------------------------------*/
#define SHOOT_OK (0)        /* 运行正常 */
#define SHOOT_ERR (-1)      /* 运行时发现了其他错误 */
#define SHOOT_ERR_NULL (-2) /* 运行时发现NULL指针 */
#define SHOOT_ERR_MODE (-3) /* 运行时配置了错误的CMD_Shoot_Mode_t */

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* 用enum组合所有PID，方便访问，配合数组使用 */
enum Shoot_Acuator_e {
  SHOOT_ACTR_FRIC1_IDX = 0, /* 1号摩擦轮相关的索引值 */
  SHOOT_ACTR_FRIC2_IDX,     /* 2号摩擦轮相关的索引值 */
  SHOOT_ACTR_TRIG_IDX,      /* 扳机电机相关的索引值 */
  SHOOT_ACTR_NUM,       /* 总共的动作器数量 */
};

/* 射击参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  const PID_Params_t fric_pid_param[2]; /* 摩擦轮电机控制PID的参数 */
  PID_Params_t trig_pid_param;          /* 扳机电机控制PID的参数 */

  struct {
    float fric;           /* 摩擦轮电机 */
    float trig;           /* 扳机电机 */
  } low_pass_cutoff_freq; /* 低通滤波器截止频率 */

  float bullet_speed_scaler; /* 子弹初速和电机转速之间的映射参数 */
  float bullet_speed_bias; /* 子弹初速和电机转速之间的映射参数 */
  float num_trig_tooth;    /* 拨弹盘中一圈能存储几颗弹丸 */
} Shoot_Params_t;

/* 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
  包含了初始化参数，中间变量，输出变量。
*/
typedef struct {
  const Shoot_Params_t
      *param; /* 射击的参数，初始化运行时都要使用，用Shoot_Init设定 */

  /* 模块通用 */

  /* TODO: 考虑放到Control中实时检测dt */
  float dt_sec;          /* 调用Shoot_Control的周期，以秒为单位， */
  CMD_Shoot_Mode_t mode; /* 射击模式 */
  osTimerId_t trig_timer_id; /* 控制拨弹电机的软件定时器 */

  struct {
    float fric_rpm[2]; /* 摩擦轮电机转速，单位：RPM */
    float trig_angle;  /* 拨弹电机角度，单位：弧度 */
  } feedback;          /* 反馈信息 */

  struct {
    float fric_rpm[2]; /* 摩擦轮电机转速，单位：RPM */
    float trig_angle;  /* 拨弹电机角度，单位：弧度 */
  } set_point;         /* PID计算的目标值 */

  struct {
    PID_t fric[2]; /* 控制摩擦轮 */
    PID_t trig;    /* 控制拨弹电机 */
  } pid;           /* 反馈控制用的PID */

  struct {
    LowPassFilter2p_t fric[2]; /* 过滤摩擦轮 */
    LowPassFilter2p_t trig;    /* 过滤拨弹电机 */
  } filter;                    /* 电机输出过滤器 */

  int8_t heat_limiter; /* 枪管热度占位变量 */

  float out[SHOOT_ACTR_NUM]; /* 输出数组，通过Shoot_Acuator_e里的值访问 */

} Shoot_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *param, float dt_sec);
int8_t Shoot_UpdateFeedback(Shoot_t *s, CAN_t *can);
int8_t Shoot_Control(Shoot_t *s, CMD_Shoot_Ctrl_t *s_ctrl);

#ifdef __cplusplus
}
#endif
