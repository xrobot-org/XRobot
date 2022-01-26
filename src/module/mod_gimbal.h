/*
 * 云台模组
 */

#pragma once

#include "comp_ahrs.h"
#include "comp_cmd.h"
#include "comp_filter.h"
#include "comp_pid.h"
#include "dev_bmi088.h"
#include "dev_motor.h"
#include "dev_referee.h"

/* GIMBAL_CTRL_NUM长度的数组都可以用这个枚举访问 */
enum gimbal_pid_e {
  GIMBAL_CTRL_YAW_OMEGA_IDX = 0, /* Yaw轴控制的角速度环控制器的索引值 */
  GIMBAL_CTRL_YAW_ANGLE_IDX, /* Yaw轴控制的角度环控制器的索引值 */
  GIMBAL_CTRL_PIT_OMEGA_IDX, /* Pitch轴控制的角速度环控制器的索引值 */
  GIMBAL_CTRL_PIT_ANGLE_IDX, /* Pitch轴控制的角度环控制器的索引值 */
  GIMBAL_CTRL_NUM,           /* 总共的控制器数量 */
};

/* GIMBAL_ACTR_NUM长度的数组都可以用这个枚举访问 */
enum gimbal_acuator_e {
  GIMBAL_ACTR_YAW_IDX = 0, /* Yaw轴动作器的索引值 */
  GIMBAL_ACTR_PIT_IDX,     /* Pitch动作器索引值 */
  GIMBAL_ACTR_NUM,         /* 总共的动作器数量 */
};

/* 云台参数的结构体，包含所有初始化用的参数，通常是const，存好几组。*/
typedef struct {
  const kpid_params_t pid[GIMBAL_CTRL_NUM]; /* 云台电机控制PID的参数 */

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

} gimbal_params_t;

/* 软件限位 */
typedef struct {
  float max;
  float min;
} gimbal_limit_t;

/* 云台反馈数据的结构体，包含反馈控制用的反馈数据 */
typedef struct {
  vector3_t gyro; /* IMU的陀螺仪数据 */

  /* 欧拉角 */
  struct {
    eulr_t imu;     /* 由IMU计算的欧拉角 */
    eulr_t encoder; /* 由编码器计算的欧拉角 */
  } eulr;
} gimbal_feedback_t;

/*
 * 运行的主结构体，所有这个文件里的函数都在操作这个结构体。
 * 包含了初始化参数，中间变量，输出变量。
 */
typedef struct {
  uint32_t lask_wakeup;
  float dt;

  const gimbal_params_t *param; /* 云台的参数，用Gimbal_Init设定 */

  /* 模块通用 */
  gimbal_mode_t mode; /* 云台模式 */

  /* PID计算的目标值 */
  struct {
    eulr_t eulr; /* 表示云台姿态的欧拉角 */
  } setpoint;

  kpid_t pid[GIMBAL_CTRL_NUM]; /* PID数组 */

  gimbal_limit_t limit;

  low_pass_filter_2p_t filter_out[GIMBAL_ACTR_NUM]; /* 输出滤波器滤波器数组 */

  float out[GIMBAL_ACTR_NUM]; /* 输出数组 */

  gimbal_feedback_t feedback; /* 反馈 */

  float scan_yaw_direction; /* 扫描模式yaw轴旋转方向 */
  float scan_pit_direction; /* 扫描模式pit轴旋转方向 */

  eulr_t *mech_zero; /* 机械零点 */

} gimbal_t;

/**
 * @brief 初始化云台
 *
 * @param g 包含云台数据的结构体
 * @param param 包含云台参数的结构体指针
 * @param target_freq 线程预期的运行频率
 */
void gimbal_init(gimbal_t *g, const gimbal_params_t *param, float limit_max,
                 eulr_t *mech_zero, float target_freq);

/**
 * @brief 通过CAN设备更新云台反馈信息
 *
 * @param g 云台
 * @param can CAN设备
 */
void gimbal_update_feedback(gimbal_t *g,
                            const motor_feedback_group_t *gimbal_motor_yaw,
                            const motor_feedback_group_t *gimbal_motor_pit);

/**
 * @brief 运行云台控制逻辑
 *
 * @param g 包含云台数据的结构体
 * @param fb 云台反馈信息
 * @param g_cmd 云台控制指令
 * @param dt_sec 两次调用的时间间隔
 */
void gimbal_control(gimbal_t *g, cmd_gimbal_t *g_cmd, uint32_t now);

/**
 * @brief 复制云台输出值
 *
 * @param g 包含云台数据的结构体
 * @param out CAN设备云台输出结构体
 */
void gimbal_pack_output(gimbal_t *g, motor_control_t *pit_out,
                        motor_control_t *yaw_out);

/**
 * @brief 导出云台UI数据
 *
 * @param g 云台结构体
 * @param ui UI结构体
 */
void gimbal_pack_ui(const gimbal_t *g, ui_gimbal_t *ui);
