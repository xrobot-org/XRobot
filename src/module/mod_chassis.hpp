/**
 * @file chassis.h
 * @author Qu Shen (503578404@qq.com)
 * @brief 底盘模组
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_mixer.hpp"
#include "comp_pid.hpp"
#include "dev_cap.hpp"
#include "dev_motor.hpp"
#include "dev_referee.hpp"
#include "dev_tof.hpp"

/* 底盘类型（底盘的物理设计） */
typedef enum {
  CHASSIS_TYPE_MECANUM,    /* 麦克纳姆轮 */
  CHASSIS_TYPE_PARLFIX4,   /* 平行摆设的四个驱动轮 */
  CHASSIS_TYPE_PARLFIX2,   /* 平行摆设的两个驱动轮 */
  CHASSIS_TYPE_OMNI_CROSS, /* 叉型摆设的四个全向轮 */
  CHASSIS_TYPE_OMNI_PLUS,  /* 十字型摆设的四个全向轮 */
  CHASSIS_TYPE_DRONE,      /* 底盘为无人机 */
  CHASSIS_TYPE_SINGLE,     /* 单个摩擦轮 */
  CHASSIS_TYPE_NONE,       /* 无底盘 */
} chassis_type_t;

/* 底盘参数的结构体，包含所有初始化用的参数，通常是const，存好几组 */
typedef struct {
  chassis_type_t type; /* 底盘类型，底盘的机械设计和轮子选型 */
  kpid_params_t motor_pid_param;  /* 轮子控制PID的参数 */
  kpid_params_t follow_pid_param; /* 跟随云台PID的参数 */

  /* 低通滤波器截止频率 */
  struct {
    float in;  /* 输入 */
    float out; /* 输出 */
  } low_pass_cutoff_freq;

  /* 电机反装 应该和云台设置相同*/
  struct {
    bool yaw;
    bool chassis_motor;
  } reverse;

} chassis_params_t;

/*
 * 运行的主结构体，所有这个文件里的函数都在操作这个结构体
 * 包含了初始化参数，中间变量，输出变量
 */
typedef struct {
  uint32_t lask_wakeup;
  float dt;

  const chassis_params_t *param; /* 底盘的参数，用Chassis_Init设定 */
  eulr_t *gimbal_mech_zero;      /* 云台机械零点 */

  /* 模块通用 */
  chassis_mode_t mode; /* 底盘模式 */

  /* 底盘设计 */
  size_t num_wheel; /* 底盘轮子数量 */
  mixer_t mixer;    /* 混合器，移动向量->电机目标值 */

  move_vector_t move_vec; /* 底盘实际的运动向量 */

  /* 反馈信息 */
  struct {
    float gimbal_yaw_encoder_angle; /* 云台Yaw轴编码器角度 */
    float *motor_rotational_speed; /* 电机转速的动态数组，单位：RPM */
    tof_feedback_t tof;
  } feedback;

  float wz_dir_mult; /* 小陀螺模式旋转方向乘数 */
  float vy_dir_mult; /* scan模式移动方向乘数 */

  /* PID计算的目标值 */
  struct {
    float *motor_rotational_speed; /* 电机转速的动态数组，单位：RPM */
  } setpoint;

  /* 反馈控制用的PID */
  struct {
    kpid_t *motor; /* 控制轮子电机用的PID的动态数组 */
    kpid_t follow; /* 跟随云台用的PID */
  } pid;

  /* 滤波器 */
  struct {
    low_pass_filter_2p_t *in;  /* 反馈值滤波器 */
    low_pass_filter_2p_t *out; /* 输出值滤波器 */
  } filter;

  /* 电机最终的输出值的动态数组 */
  struct {
    motor_control_t motor;
    cap_control_t cap;
  } out;
} chassis_t;

/**
 * @brief 初始化底盘
 *
 * @param c 包含底盘数据的结构体
 * @param param 包含底盘参数的结构体指针
 * @param target_freq 任务预期的运行频率
 */
void chassis_init(chassis_t *c, const chassis_params_t *param,
                  eulr_t *gimbal_mech_zero, float target_freq);

/**
 * @brief 更新底盘的反馈信息
 *
 * @param c 包含底盘数据的结构体
 * @param chassis_motor 底盘电机反馈
 * @param gimbal_motor 云台电机反馈
 * @param tof 距离传感器反馈
 */
void chassis_update_feedback(chassis_t *c,
                             const motor_feedback_group_t *chassis_motor,
                             const motor_feedback_group_t *gimbal_motor,
                             const tof_feedback_t *tof);

/**
 * @brief 运行底盘控制逻辑
 *
 * @param c 包含底盘数据的结构体
 * @param c_cmd 底盘控制指令
 * @param dt_sec 两次调用的时间间隔
 */
void chassis_control(chassis_t *c, const cmd_chassis_t *c_cmd, uint32_t now);

/**
 * @brief 底盘功率限制
 *
 * @param c 底盘数据
 * @param cap 电容数据
 * @param ref 裁判系统数据
 */
void chassis_power_limit(chassis_t *c, const cap_feedback_t *cap,
                         const referee_for_chassis_t *ref);

/**
 * @brief 复制底盘输出值
 *
 * @param c 包含底盘数据的结构体
 * @param out CAN设备底盘输出结构体
 */
void chassis_pack_output(const chassis_t *c, motor_control_t *motor,
                         cap_control_t *cap);

/**
 * @brief 导出底盘数据
 *
 * @param chassis 底盘数据结构体
 * @param ui UI数据结构体
 */
void chassis_pack_ui(const chassis_t *c, ui_chassis_t *ui);
