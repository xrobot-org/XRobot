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

#include "FreeRTOS.h"
#include "comp_cmd.h"
#include "comp_filter.h"
#include "comp_pid.h"
#include "dev_motor.h"
#include "dev_referee.h"

/* 用enum组合所有PID，方便访问，配合数组使用 */
enum launcher_acuator_fric_e {
  LAUNCHER_ACTR_FRIC1_IDX = 0, /* 1号摩擦轮相关的索引值 */
  LAUNCHER_ACTR_FRIC2_IDX,     /* 2号摩擦轮相关的索引值 */
  LAUNCHER_ACTR_FRIC_NUM,      /* 总共的动作器数量 */
};

enum launcher_acuator_trig_e {
  LAUNCHER_ACTR_TRIG_IDX, /* 拨弹电机相关的索引值 */
  LAUNCHER_ACTR_TRIG_NUM, /* 总共的动作器数量 */
};

enum launcher_pid_e {
  LAUNCHER_CTRL_FRIC1_SPEED_IDX = 0, /* 摩擦轮1控制的速度环控制器的索引值 */
  LAUNCHER_CTRL_FRIC2_SPEED_IDX, /* 摩擦轮2控制的速度环控制器的索引值 */
  LAUNCHER_CTRL_TRIG_SPEED_IDX, /* 拨弹电机控制的速度环控制器的索引值 */
  LAUNCHER_CTRL_TRIG_ANGLE_IDX, /* 拨弹电机控制的角度环控制器的索引值 */
  LAUNCHER_CTRL_NUM,            /* 总共的控制器数量 */
};

/* 发射机构型号 */
typedef enum {
  LAUNCHER_MODEL_17MM = 0, /* 17mm发射机构 */
  LAUNCHER_MODEL_42MM,     /* 42mm发射机构 */
} launcher_model_t;

/* 发射器参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  kpid_params_t pid_param[LAUNCHER_CTRL_NUM]; /* 电机控制PID的参数 */
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
  launcher_model_t model;     /* 发射机构型号 */
  float default_bullet_speed; /* 默认弹丸初速度 */
  uint32_t min_launch_delay;  /* 最小发射间隔(1s/最大射频) */
} launcher_params_t;

/* 热量控制 */
typedef struct {
  float heat;          /* 现在热量水平 */
  float last_heat;     /* 之前的热量水平 */
  float heat_limit;    /* 热量上限 */
  float speed_limit;   /* 弹丸初速是上限 */
  float cooling_rate;  /* 冷却速率 */
  float heat_increase; /* 每发热量增加值 */

  uint32_t available_shot; /* 热量范围内还可以发射的数量 */
} launcher_heat_ctrl_t;

/* 开火控制 */
typedef struct {
  uint32_t last_launch;    /* 上次发射器时间 单位：ms */
  bool last_fire;          /* 上次开火状态 */
  bool first_pressed_fire; /* 第一次收到开火指令 */
  uint32_t launched;       /* 已经发射的弹丸 */
  uint32_t to_launch;      /* 计划发射的弹丸 */
  uint32_t launch_delay;   /* 弹丸击发延迟 */
  float bullet_speed;      /* 弹丸初速度 */
  fire_mode_t fire_mode;
} launcher_fire_ctrl_t;

/*
 * 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
 * 包含了初始化参数，中间变量，输出变量。
 */
typedef struct {
  uint32_t lask_wakeup;
  float dt;

  const launcher_params_t *param; /* 发射器的参数，用Launcher_Init设定 */

  /* 模块通用 */
  launcher_mode_t mode; /* 发射器模式 */

  /* 反馈信息 */
  struct {
    float fric_rpm[2];      /* 摩擦轮电机转速，单位：RPM */
    float trig_motor_angle; /* 拨弹电机角度，单位：弧度 */
    float trig_angle;       /* 拨弹转盘角度，单位：弧度 */
    float trig_rpm;
  } feedback;

  /* PID计算的目标值 */
  struct {
    float fric_rpm[2]; /* 摩擦轮电机转速，单位：RPM */
    float trig_angle;  /* 拨弹电机角度，单位：弧度 */
  } setpoint;

  /* 反馈控制用的PID */
  kpid_t pid[LAUNCHER_CTRL_NUM];

  /* 过滤器 */
  struct {
    /* 反馈值滤波器 */
    struct {
      low_pass_filter_2p_t fric[LAUNCHER_ACTR_FRIC_NUM]; /* 过滤摩擦轮 */
      low_pass_filter_2p_t trig[LAUNCHER_ACTR_TRIG_NUM]; /* 过滤拨弹电机 */
    } in;

    /* 输出值滤波器 */
    struct {
      low_pass_filter_2p_t fric[LAUNCHER_ACTR_FRIC_NUM]; /* 过滤摩擦轮 */
      low_pass_filter_2p_t trig[LAUNCHER_ACTR_TRIG_NUM]; /* 过滤拨弹电机 */
    } out;
  } filter;

  launcher_heat_ctrl_t heat_ctrl;
  launcher_fire_ctrl_t fire_ctrl;

  float trig_out
      [LAUNCHER_ACTR_TRIG_NUM]; /* 输出数组，通过Launcher_Acuator_e里的值访问
                                 */
  float fric_out[LAUNCHER_ACTR_FRIC_NUM];

} launcher_t;

/**
 * @brief 初始化发射器
 *
 * @param l 包含发射器数据的结构体
 * @param param 包含发射器参数的结构体指针
 * @param target_freq 线程预期的运行频率
 */
void launcher_init(launcher_t *l, const launcher_params_t *param,
                   float target_freq);

/**
 * @brief 更新发射器的反馈信息
 *
 * @param l 包含发射器数据的结构体
 * @param can CAN设备结构体
 */
void launcher_update_feedback(
    launcher_t *l, const motor_feedback_group_t *launcher_motor_trig,
    const motor_feedback_group_t *launcher_motor_fric);

/**
 * @brief 运行发射器控制逻辑
 * @warning 鼠标点击过快（100ms内多次点击）会导致热量控制被忽略
 *
 * @param l 包含发射器数据的结构体
 * @param l_cmd 发射器控制指令
 * @param l_ref 发射器使用的裁判系统数据
 * @param now 现在时刻
 */
void launcher_control(launcher_t *l, cmd_launcher_t *l_cmd,
                      referee_for_launcher_t *l_ref, uint32_t now);

/**
 * @brief 复制发射器输出值
 *
 * @param l 包含发射器数据的结构体
 * @param out CAN设备发射器输出结构体
 */
void launcher_pack_output(launcher_t *l, motor_control_t *trig_out,
                          motor_control_t *fric_out);

/**
 * @brief 导出发射器UI数据
 *
 * @param l 发射器结构体
 * @param ui UI结构体
 */
void launcher_pack_ui(launcher_t *l, ui_launcher_t *ui);
