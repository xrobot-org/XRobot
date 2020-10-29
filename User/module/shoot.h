/*
 * 射击模组
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>

#include "component\cmd.h"
#include "component\filter.h"
#include "component\pid.h"
#include "device\can.h"

/* Exported constants ------------------------------------------------------- */
#define SHOOT_OK (0)        /* 运行正常 */
#define SHOOT_ERR (-1)      /* 运行时发现了其他错误 */
#define SHOOT_ERR_NULL (-2) /* 运行时发现NULL指针 */
#define SHOOT_ERR_MODE (-3) /* 运行时配置了错误的CMD_ShootMode_t */

/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

/* 用enum组合所有PID，方便访问，配合数组使用 */
enum Shoot_Acuator_e {
  SHOOT_ACTR_FRIC1_IDX = 0, /* 1号摩擦轮相关的索引值 */
  SHOOT_ACTR_FRIC2_IDX,     /* 2号摩擦轮相关的索引值 */
  SHOOT_ACTR_TRIG_IDX,      /* 扳机电机相关的索引值 */
  SHOOT_ACTR_NUM,           /* 总共的动作器数量 */
};

/* 射击参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  KPID_Params_t fric_pid_param; /* 摩擦轮电机控制PID的参数 */
  KPID_Params_t trig_pid_param; /* 扳机电机控制PID的参数 */

  struct {
    struct {
      float fric; /* 摩擦轮电机 */
      float trig; /* 扳机电机 */
    } in;         /* 输入 */

    struct {
      float fric;         /* 摩擦轮电机 */
      float trig;         /* 扳机电机 */
    } out;                /* 输出 */
  } low_pass_cutoff_freq; /* 低通滤波器截止频率 */

  float bullet_speed_scaler; /* 子弹初速和电机转速之间的映射参数 */
  float bullet_speed_bias; /* 子弹初速和电机转速之间的映射参数 */
  float num_trig_tooth;    /* 拨弹盘中一圈能存储几颗弹丸 */
} Shoot_Params_t;

/*
 * 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
 * 包含了初始化参数，中间变量，输出变量。
 */
typedef struct {
  const Shoot_Params_t *param; /* 射击的参数，用Shoot_Init设定 */

  /* 模块通用 */
  CMD_ShootMode_t mode;      /* 射击模式 */
  osTimerId_t trig_timer_id; /* 控制拨弹电机的软件定时器 */

  struct {
    float fric_rpm[2]; /* 摩擦轮电机转速，单位：RPM */
    float trig_angle;  /* 拨弹电机角度，单位：弧度 */
  } feedback;          /* 反馈信息 */

  struct {
    float fric_rpm[2]; /* 摩擦轮电机转速，单位：RPM */
    float trig_angle;  /* 拨弹电机角度，单位：弧度 */
  } setpoint;          /* PID计算的目标值 */

  struct {
    KPID_t fric[2]; /* 控制摩擦轮 */
    KPID_t trig;    /* 控制拨弹电机 */
  } pid;            /* 反馈控制用的PID */

  struct {
    struct {
      LowPassFilter2p_t fric[2]; /* 过滤摩擦轮 */
      LowPassFilter2p_t trig;    /* 过滤拨弹电机 */
    } in;                        /* 反馈值滤波器 */
    struct {
      LowPassFilter2p_t fric[2]; /* 过滤摩擦轮 */
      LowPassFilter2p_t trig;    /* 过滤拨弹电机 */
    } out;                       /* 输出值滤波器 */
  } filter;                      /* 过滤器 */

  int8_t heat_limiter; /* 枪管热度占位变量 */
  float trig_angle; /* 拨弹转盘角度 */
  float out[SHOOT_ACTR_NUM]; /* 输出数组，通过Shoot_Acuator_e里的值访问 */

} Shoot_t;

/* Exported functions prototypes -------------------------------------------- */

/*!
 * \brief 初始化射击
 *
 * \param s 包含射击数据的结构体
 * \param param 包含射击参数的结构体指针
 * \param target_freq 任务预期的运行频率
 *
 * \return 函数运行结果
 */
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *param, float target_freq);

/*!
 * \brief 更新射击的反馈信息
 *
 * \param s 包含射击数据的结构体
 * \param can CAN设备结构体
 *
 * \return 函数运行结果
 */
int8_t Shoot_UpdateFeedback(Shoot_t *s, const CAN_t *can);

/*!
 * \brief 运行射击控制逻辑
 *
 * \param s 包含射击数据的结构体
 * \param s_cmd 射击控制指令
 * \param dt_sec 两次调用的时间间隔
 *
 * \return 函数运行结果
 */
int8_t Shoot_Control(Shoot_t *s, CMD_ShootCmd_t *s_cmd, float dt_sec);

#ifdef __cplusplus
}
#endif
