/*
  云台模组
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include "component\ahrs.h"
#include "component\cmd.h"
#include "component\filter.h"
#include "component\pid.h"
#include "device\bmi088.h"
#include "device\can.h"

/* Exported constants ------------------------------------------------------- */
#define GIMBAL_OK (0)        /* 运行正常 */
#define GIMBAL_ERR (-1)      /* 运行时发现了其他错误 */
#define GIMBAL_ERR_NULL (-2) /* 运行时发现NULL指针 */
#define GIMBAL_ERR_MODE (-3) /* 运行时配置了错误的CMD_GimbalMode_t */

/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

/* 用enum组合所有PID，GIMBAL_PID_NUM长度的数组都可以用这个枚举访问 */
enum Gimbal_PID_e {
  GIMBAL_PID_YAW_OMEGA_IDX = 0, /* Yaw轴控制的角速度环PID的索引值 */
  GIMBAL_PID_YAW_ANGLE_IDX,     /* Yaw轴控制的角度环PID的索引值 */
  GIMBAL_PID_PIT_OMEGA_IDX, /* Pitch轴控制的角速度环PID的索引值 */
  GIMBAL_PID_PIT_ANGLE_IDX, /* Pitch轴控制的角度环PID的索引值 */
  GIMBAL_PID_REL_YAW_IDX,   /* 通过编码器控制时Yaw轴PID的索引值 */
  GIMBAL_PID_REL_PIT_IDX, /* 通过编码器控制时Pitch轴PID的索引值 */
  GIMBAL_PID_NUM,         /* 总共的PID数量 */
};

/* 用enum组合所有输出，GIMBAL_ACTR_NUM长度的数组都可以用这个枚举访问 */
enum Gimbal_Acuator_e {
  GIMBAL_ACTR_YAW_IDX = 0, /* Yaw轴控制相关的索引值 */
  GIMBAL_ACTR_PIT_IDX,     /* Pitch轴控制相关的索引值 */
  GIMBAL_ACTR_NUM,         /* 总共的动作器数量 */
};

/* 云台参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  const KPID_Params_t pid[GIMBAL_PID_NUM]; /* 云台电机控制PID的参数 */

  struct {
    float out;            /* 电机输出 */
    float gyro;           /* 陀螺仪数据 */
  } low_pass_cutoff_freq; /* 低通滤波器截止频率 */

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

/* 云台反馈数据的结构体，包含反馈控制用的反馈数据 */
typedef struct {
  AHRS_Gyro_t gyro; /* IMU的陀螺仪数据 */

  struct {
    AHRS_Eulr_t imu;     /* 由IMU计算的欧拉角 */
    AHRS_Eulr_t encoder; /* 由编码器计算的欧拉角 */
  } eulr;                /* 欧拉角 */
} Gimbal_Feedback;

/* 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
  包含了初始化参数，中间变量，输出变量。
*/
typedef struct {
  const Gimbal_Params_t *param; /* 云台的参数，用Gimbal_Init设定 */

  /* 模块通用 */
  CMD_GimbalMode_t mode; /* 云台模式 */

  struct {
    AHRS_Eulr_t eulr; /* 表示云台姿态的欧拉角 */
  } setpoint;         /* PID计算的目标值 */

  KPID_t pid[GIMBAL_PID_NUM]; /* PID数组 */

  LowPassFilter2p_t filter_out[GIMBAL_ACTR_NUM]; /* 输出滤波器滤波器数组 */

  float out[GIMBAL_ACTR_NUM]; /* 输出数组 */

} Gimbal_t;

/* Exported functions prototypes -------------------------------------------- */
int8_t Gimbal_CANtoFeedback(Gimbal_Feedback *gimbal_feedback, const CAN_t *can);

int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param,
                   float target_freq);
int8_t Gimbal_Control(Gimbal_t *g, Gimbal_Feedback *fb, CMD_GimbalCmd_t *g_cmd,
                      float dt_sec);

#ifdef __cplusplus
}
#endif
