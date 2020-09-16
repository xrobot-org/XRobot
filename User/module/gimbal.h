/*
  云台模组
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "component\ahrs.h"
#include "component\cmd.h"
#include "component\filter.h"
#include "component\pid.h"
#include "device\bmi088.h"
#include "device\can.h"

/* Exported constants --------------------------------------------------------*/
#define GIMBAL_OK (0)        /* 运行正常 */
#define GIMBAL_ERR (-1)      /* 运行时发现了其他错误 */
#define GIMBAL_ERR_NULL (-2) /* 运行时发现NULL指针 */
#define GIMBAL_ERR_MODE (-3) /* 运行时配置了错误的CMD_Gimbal_Mode_t */

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* 用enum组合所有PID，方便访问，配合数组使用 */
enum Gimbal_PID_e {
  GIMBAL_PID_YAW_IN_IDX = 0, /* Yaw轴控制的内环PID的索引值 */
  GIMBAL_PID_YAW_OUT_IDX,    /* Yaw轴控制的外环PID的索引值 */
  GIMBAL_PID_PIT_IN_IDX,     /* Pitch轴控制的内环PID的索引值 */
  GIMBAL_PID_PIT_OUT_IDX,    /* Pitch轴控制的外环PID的索引值 */
  GIMBAL_PID_REL_YAW_IDX,    /* 通过编码器控制时Yaw轴PID的索引值 */
  GIMBAL_PID_REL_PIT_IDX,    /*通过编码器控制时Pitch轴PID的索引值 */
  GIMBAL_PID_NUM,        /* 总共的PID数量 */
};

/* 用enum组合所有输出，方便访问，配合数组使用 */
enum Gimbal_Acuator_e {
  GIMBAL_ACTR_YAW_IDX = 0, /* Yaw轴控制相关的索引值 */
  GIMBAL_ACTR_PIT_IDX,     /* Pitch轴控制相关的索引值 */
  GIMBAL_ACTR_NUM,     /* 总共的动作器数量 */
};

/* 云台参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  const PID_Params_t pid[GIMBAL_PID_NUM]; /* 云台电机控制PID的参数 */
  float low_pass_cutoff_freq;             /* 低通滤波器截止频率 */
  struct {
    struct {
      float high; /* 限位高点 */
      float low;  /* 限位低点 */
    } pitch;      /* Pitch轴限位 */
  } limit;        /* 软件限位 */

  /* TODO: 能使用命令行修改 */
  struct {
    float yaw;
    float pit;
  } encoder_center; /* 云台编码器中间位置 */
} Gimbal_Params_t;

/* 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
  包含了初始化参数，中间变量，输出变量。
*/
typedef struct {
  const Gimbal_Params_t
      *param; /* 云台的参数，初始化运行时都要使用，用Gimbal_Init设定 */

  /* 模块通用 */
  /* TODO: 考虑放到Control中实时检测dt */
  float dt_sec; /* 调用Gimbal_Control的周期，以秒为单位， */
  CMD_Gimbal_Mode_t mode; /* 云台模式 */

  struct {
    AHRS_Gyro_t gyro;

    struct {
      AHRS_Eulr_t imu;     /* 由IMU计算的欧拉角 */
      AHRS_Eulr_t encoder; /* 由编码器计算的欧拉角 */
    } eulr;                /* 欧拉角 */
  } feedback;              /* 反馈信息 */

  struct {
    AHRS_Eulr_t eulr; /* 表示云台姿态的欧拉角 */
  } set_point;        /* PID计算的目标值 */

  PID_t pid[GIMBAL_PID_NUM]; /* PID数组，通过Gimbal_PID_e里的值访问 */

  LowPassFilter2p_t
      filter[GIMBAL_ACTR_NUM]; /* 滤波器数组，通过Gimbal_Acuator_e里的值访问 */

  float out[GIMBAL_ACTR_NUM]; /* 输出数组，通过Gimbal_Acuator_e里的值访问 */

} Gimbal_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param, float dt_sec);
int8_t Gimbal_UpdateFeedback(Gimbal_t *g, CAN_t *can);
int8_t Gimbal_Control(Gimbal_t *g, CMD_Gimbal_Ctrl_t *g_ctrl);

#ifdef __cplusplus
}
#endif
