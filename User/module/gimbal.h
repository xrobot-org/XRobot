/*
 * 云台模组
 */

#pragma once

/* Includes ----------------------------------------------------------------- */
#include "component/ahrs.h"
#include "component/cmd.h"
#include "component/filter.h"
#include "component/pid.h"
#include "device/bmi088.h"
#include "device/can.h"
#include "device/referee.h"

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
  GIMBAL_PID_NUM,           /* 总共的PID数量 */
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

  /* 低通滤波器截止频率 */
  struct {
    float out;  /* 电机输出 */
    float gyro; /* 陀螺仪数据 */
  } low_pass_cutoff_freq;

  float pitch_travel_rad; /* 云台pitch轴行程弧度 */

  /* 设置默认运动方向 */
  struct {
    bool yaw;
    bool pit;
  } reverse;

} Gimbal_Params_t;

/* 软件限位 */
typedef struct {
  float max;
  float min;
} Gimbal_Limit_t;

/* 云台反馈数据的结构体，包含反馈控制用的反馈数据 */
typedef struct {
  AHRS_Gyro_t gyro; /* IMU的陀螺仪数据 */

  /* 欧拉角 */
  struct {
    AHRS_Eulr_t imu;     /* 由IMU计算的欧拉角 */
    AHRS_Eulr_t encoder; /* 由编码器计算的欧拉角 */
  } eulr;
} Gimbal_Feedback_t;

/*
 * 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
 * 包含了初始化参数，中间变量，输出变量。
 */
typedef struct {
  uint32_t lask_wakeup;
  float dt;

  const Gimbal_Params_t *param; /* 云台的参数，用Gimbal_Init设定 */

  /* 模块通用 */
  Game_GimbalMode_t mode; /* 云台模式 */

  /* PID计算的目标值 */
  struct {
    AHRS_Eulr_t eulr; /* 表示云台姿态的欧拉角 */
  } setpoint;

  KPID_t pid[GIMBAL_PID_NUM]; /* PID数组 */

  Gimbal_Limit_t limit;

  LowPassFilter2p_t filter_out[GIMBAL_ACTR_NUM]; /* 输出滤波器滤波器数组 */

  float out[GIMBAL_ACTR_NUM]; /* 输出数组 */

  Gimbal_Feedback_t feedback; /* 反馈 */

} Gimbal_t;

/* Exported functions prototypes -------------------------------------------- */

/**
 * \brief 初始化云台
 *
 * \param g 包含云台数据的结构体
 * \param param 包含云台参数的结构体指针
 * \param target_freq 任务预期的运行频率
 *
 * \return 函数运行结果
 */
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param, float limit,
                   float target_freq);

/**
 * \brief 通过CAN设备更新云台反馈信息
 *
 * \param g 云台
 * \param can CAN设备
 *
 * \return 函数运行结果
 */
int8_t Gimbal_UpdateFeedback(Gimbal_t *g, const CAN_t *can);

/**
 * \brief 运行云台控制逻辑
 *
 * \param g 包含云台数据的结构体
 * \param fb 云台反馈信息
 * \param g_cmd 云台控制指令
 * \param dt_sec 两次调用的时间间隔
 *
 * \return 函数运行结果
 */
int8_t Gimbal_Control(Gimbal_t *g, CMD_GimbalCmd_t *g_cmd, uint32_t now);

/**
 * \brief 复制云台输出值
 *
 * \param s 包含云台数据的结构体
 * \param out CAN设备云台输出结构体
 */
void Gimbal_PackOutput(Gimbal_t *g, CAN_GimbalOutput_t *out);

/**
 * \brief 清空输出值
 *
 * \param output 要清空的结构体
 */
void Gimbal_ResetOutput(CAN_GimbalOutput_t *output);

/**
 * @brief 导出云台UI数据
 *
 * @param g 云台结构体
 * @param ui UI结构体
 */
void Gimbal_PackUi(const Gimbal_t *g, Referee_GimbalUI_t *ui);
