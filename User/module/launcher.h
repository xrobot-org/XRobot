/**
 * @file launcher.h
 * @author Qu Shen (503578404@qq.com)
 * @brief 弹丸发射器模块
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
enum Launcher_Acuator_e {
  LAUNCHER_ACTR_FRIC1_IDX = 0, /* 1号摩擦轮相关的索引值 */
  LAUNCHER_ACTR_FRIC2_IDX,     /* 2号摩擦轮相关的索引值 */
  LAUNCHER_ACTR_TRIG_IDX,      /* 拨弹电机相关的索引值 */
  LAUNCHER_ACTR_NUM,           /* 总共的动作器数量 */
};

/* 发射机构型号 */
typedef enum {
  LAUNCHER_MODEL_17MM = 0, /* 17mm发射机构 */
  LAUNCHER_MODEL_42MM,     /* 42mm发射机构 */
} Launcher_Model_t;

/* 发射器参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  KPID_Params_t fric_pid_param; /* 摩擦轮电机控制PID的参数 */
  KPID_Params_t trig_pid_param; /* 拨弹电机控制PID的参数 */
  /* 低通滤波器截止频率 */
  struct {
    /* 输入 */
    struct {
      float fric; /* 摩擦轮电机 */
      float trig; /* 拨弹电机 */
    } in;

    /* 输出 */
    struct {
      float fric; /* 摩擦轮电机 */
      float trig; /* 拨弹电机 */
    } out;
  } low_pass_cutoff_freq;

  float num_trig_tooth;       /* 拨弹盘中一圈能存储几颗弹丸 */
  float trig_gear_ratio;      /* 拨弹电机减速比 3508:19, 2006:36 */
  float fric_radius;          /* 摩擦轮半径，单位：米 */
  float cover_open_duty;      /* 弹舱盖打开时舵机PWM占空比 */
  float cover_close_duty;     /* 弹舱盖关闭时舵机PWM占空比 */
  Launcher_Model_t model;     /* 发射机构型号 */
  float default_bullet_speed; /* 默认弹丸初速度 */
  uint32_t min_launch_delay;  /* 最小发射间隔(1s/最大射频) */
} Launcher_Params_t;

/* 热量控制 */
typedef struct {
  float heat;          /* 现在热量水平 */
  float last_heat;     /* 之前的热量水平 */
  float heat_limit;    /* 热量上限 */
  float speed_limit;   /* 弹丸初速是上限 */
  float cooling_rate;  /* 冷却速率 */
  float heat_increase; /* 每发热量增加值 */

  uint32_t available_shot; /* 热量范围内还可以发射的数量 */
} Launcher_HeatCtrl_t;

/* 开火控制 */
typedef struct {
  uint32_t last_launch;    /* 上次发射器时间 单位：ms */
  bool last_fire;          /* 上次开火状态 */
  bool first_pressed_fire; /* 第一次收到开火指令 */
  uint32_t launched;       /* 已经发射的弹丸 */
  uint32_t to_launch;      /* 计划发射的弹丸 */
  uint32_t launch_delay;   /* 弹丸击发延迟 */
  float bullet_speed;      /* 弹丸初速度 */
  Game_FireMode_t fire_mode;
} Launcher_FireCtrl_t;

/*
 * 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
 * 包含了初始化参数，中间变量，输出变量。
 */
typedef struct {
  uint32_t lask_wakeup;
  float dt;

  const Launcher_Params_t *param; /* 发射器的参数，用Launcher_Init设定 */

  /* 模块通用 */
  Game_LauncherMode_t mode; /* 发射器模式 */

  /* 反馈信息 */
  struct {
    float fric_rpm[2];      /* 摩擦轮电机转速，单位：RPM */
    float trig_motor_angle; /* 拨弹电机角度，单位：弧度 */
    float trig_angle;       /* 拨弹转盘角度，单位：弧度 */
  } feedback;

  /* PID计算的目标值 */
  struct {
    float fric_rpm[2]; /* 摩擦轮电机转速，单位：RPM */
    float trig_angle;  /* 拨弹电机角度，单位：弧度 */
  } setpoint;

  /* 反馈控制用的PID */
  struct {
    KPID_t fric[2]; /* 控制摩擦轮 */
    KPID_t trig;    /* 控制拨弹电机 */
  } pid;

  /* 过滤器 */
  struct {
    /* 反馈值滤波器 */
    struct {
      LowPassFilter2p_t fric[2]; /* 过滤摩擦轮 */
      LowPassFilter2p_t trig;    /* 过滤拨弹电机 */
    } in;

    /* 输出值滤波器 */
    struct {
      LowPassFilter2p_t fric[2]; /* 过滤摩擦轮 */
      LowPassFilter2p_t trig;    /* 过滤拨弹电机 */
    } out;
  } filter;

  Launcher_HeatCtrl_t heat_ctrl;
  Launcher_FireCtrl_t fire_ctrl;

  float out[LAUNCHER_ACTR_NUM]; /* 输出数组，通过Launcher_Acuator_e里的值访问 */

} Launcher_t;

/* Exported functions prototypes -------------------------------------------- */

/**
 * @brief 初始化发射器
 *
 * @param l 包含发射器数据的结构体
 * @param param 包含发射器参数的结构体指针
 * @param target_freq 任务预期的运行频率
 */
void Launcher_Init(Launcher_t *l, const Launcher_Params_t *param,
                   float target_freq);

/**
 * @brief 更新发射器的反馈信息
 *
 * @param l 包含发射器数据的结构体
 * @param can CAN设备结构体
 */
void Launcher_UpdateFeedback(Launcher_t *l, const CAN_t *can);

/**
 * @brief 运行发射器控制逻辑
 * @warning 鼠标点击过快（100ms内多次点击）会导致热量控制被忽略
 *
 * @param l 包含发射器数据的结构体
 * @param l_cmd 发射器控制指令
 * @param l_ref 发射器使用的裁判系统数据
 * @param now 现在时刻
 */
void Launcher_Control(Launcher_t *l, CMD_LauncherCmd_t *l_cmd,
                      Referee_ForLauncher_t *l_ref, uint32_t now);

/**
 * @brief 复制发射器输出值
 *
 * @param l 包含发射器数据的结构体
 * @param out CAN设备发射器输出结构体
 */
void Launcher_PackOutput(Launcher_t *l, CAN_LauncherOutput_t *out);

/**
 * @brief 清空输出值
 *
 * @param output 要清空的结构体
 */
void Launcher_ResetOutput(CAN_LauncherOutput_t *output);

/**
 * @brief 导出发射器UI数据
 *
 * @param l 发射器结构体
 * @param ui UI结构体
 */
void Launcher_PackUi(Launcher_t *l, UI_LauncherUI_t *ui);
