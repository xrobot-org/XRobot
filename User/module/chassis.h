/*
  底盘模组
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include "component\cmd.h"
#include "component\filter.h"
#include "component\mixer.h"
#include "component\pid.h"
#include "device\can.h"

/* Exported constants ------------------------------------------------------- */
#define CHASSIS_OK (0)        /* 运行正常 */
#define CHASSIS_ERR (-1)      /* 运行时发现了其他错误 */
#define CHASSIS_ERR_NULL (-2) /* 运行时发现NULL指针 */
#define CHASSIS_ERR_MODE (-3) /* 运行时配置了错误的CMD_ChassisMode_t */
#define CHASSIS_ERR_TYPE (-4) /* 运行时配置了错误的Chassis_Type_t */

/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
typedef enum {
  CHASSIS_TYPE_MECANUM,    /* 麦克纳姆轮 */
  CHASSIS_TYPE_PARLFIX4,   /* 平行摆设的四个驱动轮 */
  CHASSIS_TYPE_PARLFIX2,   /* 平行摆设的两个驱动轮 */
  CHASSIS_TYPE_OMNI_CROSS, /* 叉型摆设的四个全向轮 */
  CHASSIS_TYPE_OMNI_PLUS,  /* 十字型摆设的四个全向轮 */
  CHASSIS_TYPE_DRONE,      /* 底盘为无人机 */
} Chassis_Type_t;

/* 底盘参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  Chassis_Type_t type; /* 底盘类型，底盘的机械设计和轮子选型 */
  KPID_Params_t motor_pid_param;  /* 轮子控制PID的参数 */
  KPID_Params_t follow_pid_param; /* 跟随云台PID的参数 */

  struct {
    float in;             /* 输入 */
    float out;            /* 输出 */
  } low_pass_cutoff_freq; /* 低通滤波器截止频率 */

} Chassis_Params_t;

/* 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
  包含了初始化参数，中间变量，输出变量。
*/
typedef struct {
  const Chassis_Params_t *param; /* 底盘的参数，用Chassis_Init设定 */

  /* 模块通用 */
  CMD_ChassisMode_t mode; /* 底盘模式 */

  /* 底盘设计 */
  int8_t num_wheel; /* 底盘轮子数量 */
  Mixer_t mixer;    /* 混合器，移动向量->电机目标值 */

  MoveVector_t move_vec; /* 底盘实际的运动向量 */

  struct {
    float gimbal_yaw_angle; /* 云台Yaw轴编码器角度 */
    float *motor_rpm;       /* 电机转速的动态数组，单位：RPM */
  } feedback;               /* 反馈信息 */

  struct {
    float *motor_rpm; /* 电机转速的动态数组，单位：RPM */
  } setpoint;         /* PID计算的目标值 */

  struct {
    KPID_t *motor; /* 控制轮子电机用的PID的动态数组 */
    KPID_t follow; /* 跟随云台用的PID */
  } pid;           /* 反馈控制用的PID */

  struct {
    LowPassFilter2p_t *in;  /* 反馈值滤波器 */
    LowPassFilter2p_t *out; /* 输出值滤波器 */
  } filter;                 /* 滤波器 */

  float *out; /* 电机最终的输出值的动态数组 */

} Chassis_t;

/* Exported functions prototypes -------------------------------------------- */
/* Chassis_Control的目标运行频率 */
int8_t Chassis_Init(Chassis_t *c, const Chassis_Params_t *param,
                    float target_freq);
int8_t Chassis_UpdateFeedback(Chassis_t *c, const CAN_t *can);
int8_t Chassis_Control(Chassis_t *c, CMD_ChassisCmd_t *c_cmd, float dt_sec);

#ifdef __cplusplus
}
#endif
