/**
 * @file launcher.h
 * @author Qu Shen (503578404@qq.com)
 * @brief 小弹丸飞镖发射器模块
 * @version 1.0.0
 * @date 2021-05-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>

#include "component/cmd.h"
#include "component/filter.h"
#include "component/pid.h"
#include "device/can.h"
#include "device/referee.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

/* 用enum组合所有PID，方便访问，配合数组使用 */
enum DartLauncher_Friction_e {
  DART_LAUNCHER_FRIC1_IDX = 0, /* 1号摩擦轮相关的索引值 */
  DART_LAUNCHER_FRIC2_IDX,     /* 2号摩擦轮相关的索引值 */
  DART_LAUNCHER_FRIC3_IDX,     /* 3号摩擦轮相关的索引值 */
  DART_LAUNCHER_FRIC4_IDX,     /* 4号摩擦轮相关的索引值 */
  DART_LAUNCHER_FRIC_NUM,      /* 总共的动作器数量 */
};

enum DartLauncher_Fly_e {
  DART_LAUNCHER_FLY1_IDX, /* 1号飞轮电机相关的索引值 */
  DART_LAUNCHER_FLY2_IDX, /* 2号飞轮电机相关的索引值 */
  DART_LAUNCHER_FLY_NUM,  /* 总共的动作器数量 */
};

/* 飞镖发射器参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  KPID_Params_t fric_pid_param; /* 摩擦轮电机控制PID的参数 */
  KPID_Params_t fly_pid_param;  /* 飞轮电机控制PID的参数 */
  KPID_Params_t feed_pid_param; /* 供弹电机控制PID的参数 */

  /* 低通滤波器截止频率 */
  struct {
    /* 输入 */
    struct {
      float fric; /* 摩擦轮电机 */
      float fly;  /* 飞轮电机 */
      float feed; /* 供弹电机 */
    } in;

    /* 输出 */
    struct {
      float fric; /* 摩擦轮电机 */
      float fly;  /* 飞轮电机 */
      float feed; /* 供弹电机 */
    } out;
  } low_pass_cutoff_freq;

  float fric_radius; /* 摩擦轮半径，单位：米 */
  float fly_radius;  /* 飞轮半径，单位：米 */
  float feed_radius; /* 供弹轮半径，单位：米 */
} DartLauncher_Params_t;

/*
 * 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
 * 包含了初始化参数，中间变量，输出变量。
 */
typedef struct {
  uint32_t lask_wakeup;
  float dt;

  /* 飞镖发射器的参数，用DartLauncher_Init设定 */
  const DartLauncher_Params_t *param;

  /* 模块通用 */
  Game_DartLauncherMode_t mode; /* 飞镖发射器模式 */

  /* 反馈信息 */
  struct {
    float fric_rpm[DART_LAUNCHER_FRIC_NUM]; /* 摩擦轮电机转速，单位：RPM */
    float fly_rpm[DART_LAUNCHER_FLY_NUM]; /* 飞轮电机转速，单位：RPM */
    float feed_angle; /* 供弹电机角度，单位：弧度 */
    float feed_pos;   /* 供弹链路位置，单位：TODO */
  } feedback;

  /* PID计算的目标值 */
  struct {
    float fric_rpm[DART_LAUNCHER_FRIC_NUM]; /* 摩擦轮电机转速，单位：RPM */
    float fly_rpm[DART_LAUNCHER_FLY_NUM]; /* 飞轮电机转速，单位：RPM */
    float feed_pos; /* 供弹链路位置，单位：TODO */
  } setpoint;

  /* 反馈控制用的PID */
  struct {
    KPID_t fric[DART_LAUNCHER_FRIC_NUM]; /* 控制摩擦轮电机 */
    KPID_t fly[DART_LAUNCHER_FLY_NUM];   /* 控制飞轮电机 */
    KPID_t feed;                         /* 控制供弹电机 */
  } pid;

  /* 过滤器 */
  struct {
    /* 反馈值滤波器 */
    struct {
      LowPassFilter2p_t fric[DART_LAUNCHER_FRIC_NUM]; /* 过滤摩擦轮 */
      LowPassFilter2p_t fly[DART_LAUNCHER_FLY_NUM];   /* 过滤飞轮电机 */
      LowPassFilter2p_t feed;                         /* 过滤供弹电机 */
    } in;

    /* 输出值滤波器 */
    struct {
      LowPassFilter2p_t fric[DART_LAUNCHER_FRIC_NUM]; /* 过滤摩擦轮 */
      LowPassFilter2p_t fly[DART_LAUNCHER_FLY_NUM];   /* 过滤飞轮电机 */
      LowPassFilter2p_t feed;                         /* 过滤供弹电机 */
    } out;
  } filter;

  /* 输出 */
  struct {
    float fric[DART_LAUNCHER_FRIC_NUM];
    float fly[DART_LAUNCHER_FLY_NUM];
    float feed;
  } out;

} DartLauncher_t;

/* Exported functions prototypes -------------------------------------------- */

/**
 * @brief 初始化飞镖发射器
 *
 * @param dl 包含飞镖发射器数据的结构体
 * @param param 包含飞镖发射器参数的结构体指针
 * @param target_freq 任务预期的运行频率
 */
void DartLauncher_Init(DartLauncher_t *dl, const DartLauncher_Params_t *param,
                       float target_freq);

/**
 * @brief 更新飞镖发射器的反馈信息
 *
 * @param dl 包含飞镖发射器数据的结构体
 * @param can CAN设备结构体
 */
void DartLauncher_UpdateFeedback(DartLauncher_t *dl, const CAN_t *can);

/**
 * @brief 运行飞镖发射器控制逻辑
 * @warning 鼠标点击过快（100ms内多次点击）会导致热量控制被忽略
 *
 * @param dl 包含飞镖发射器数据的结构体
 * @param l_cmd 飞镖发射器控制指令
 * @param l_ref 飞镖发射器使用的裁判系统数据
 * @param now 现在时刻
 */
void DartLauncher_Control(DartLauncher_t *dl, Referee_ForDartLauncher_t *dl_ref,
                          uint32_t now);

/**
 * @brief 复制飞镖发射器输出值
 *
 * @param dl 包含飞镖发射器数据的结构体
 * @param out CAN设备飞镖发射器输出结构体
 */
void DartLauncher_PackOutput(DartLauncher_t *dl, CAN_DartLauncherOutput_t *out);

/**
 * @brief 清空输出值
 *
 * @param output 要清空的结构体
 */
void DartLauncher_ResetOutput(CAN_DartLauncherOutput_t *output);