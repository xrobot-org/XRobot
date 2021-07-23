/**
 * @file dart.h
 * @author Qu Shen
 * @brief 飞镖控制模组
 * @version 0.1
 * @date 2021-07-11
 *
 * @copyright Copyright (c) 2021
 *
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
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

/* 飞镖行为状态 */
typedef enum {
  DART_STAGE_READY,   /* 准备状态 */
  DART_STAGE_ASCEND,  /* 上升状态 */
  DART_STAGE_DESCEND, /* 下降状态 */
  DART_STAGE_LAND     /* 着陆状态 */
} Dart_Stage_t;

/* Elevon指飞翼中控制pitch和roll的舵面
 * Tailerons指战斗机中控制roll和pitch的全动水平舵面 */

/* 飞镖动作器 */
typedef enum {
  DART_ACTR_ELEVON = 0, /* 升降副翼 */
  DART_ACTR_RUDDER,     /* 方向舵 */
  DART_ACTR_NUM
} Dart_Actuator_t;

/* 飞镖参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  const KPID_Params_t pid; /* 飞镖电机控制PID的参数 */
} Dart_Params_t;

/* 飞镖反馈数据的结构体，包含反馈控制用的反馈数据 */
typedef struct {
  Vector3_t gyro; /* IMU的陀螺仪数据 */

  /* 欧拉角 */
  struct {
    AHRS_Eulr_t imu; /* 由IMU计算的欧拉角 */
  } eulr;
} Dart_Feedback_t;

/*
 * 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
 * 包含了初始化参数，中间变量，输出变量。
 */
typedef struct {
  uint32_t lask_wakeup;
  float dt;

  const Dart_Params_t *param; /* 飞镖的参数，用Dart_Init设定 */

  Dart_Stage_t stage; /* 控制阶段 */

  /* PID计算的目标值 */
  struct {
    AHRS_Eulr_t eulr; /* 表示飞镖姿态的欧拉角 */
  } setpoint;

  KPID_t pid; /* PID数组 */

  LowPassFilter2p_t filter_in; /* 输出滤波器滤波器数组 */

  float out[DART_ACTR_NUM]; /* 输出数组 */

  Dart_Feedback_t feedback; /* 反馈 */

} Dart_t;

/* Exported functions prototypes -------------------------------------------- */

/**
 * @brief 初始化飞镖
 *
 * @param d 包含飞镖数据的结构体
 * @param param 包含飞镖参数的结构体指针
 * @param target_freq 任务预期的运行频率
 *
 * @return 函数运行结果
 */
void Dart_Init(Dart_t *d, const Dart_Params_t *param, float limit,
               float target_freq);

/**
 * @brief 通过CAN设备更新飞镖反馈信息
 *
 * @param d 飞镖
 * @param can CAN设备
 *
 * @return 函数运行结果
 */
void Dart_UpdateFeedback(Dart_t *d, const CAN_t *can);

/**
 * @brief 运行飞镖控制逻辑
 *
 * @param d 包含飞镖数据的结构体
 * @param fb 飞镖反馈信息
 * @param g_cmd 飞镖控制指令
 * @param dt_sec 两次调用的时间间隔
 *
 * @return 函数运行结果
 */
void Dart_Control(Dart_t *d, CMD_DartCmd_t *g_cmd, uint32_t now);
