/*
 * 云台模组
 */

#include "mod_gimbal.h"

#include <stdlib.h>

/**
 * @brief 设置云台模式
 *
 * @param g 包含云台数据的结构体
 * @param mode 要设置的模式
 */
static void gimbal_set_mode(gimbal_t *g, gimbal_mode_t mode) {
  ASSERT(g);
  if (mode == g->mode) return;

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < g->num_yaw; i++) {
    kpid_reset(g->pid[GIMBAL_CTRL_YAW_OMEGA_IDX] + i);
  }

  for (size_t i = 0; i < g->num_pit; i++) {
    kpid_reset(g->pid[GIMBAL_CTRL_PIT_OMEGA_IDX] + i);
  }

  for (size_t i = 0; i < g->num_yaw; i++) {
    low_pass_filter_2p_reset(g->filter_out[GIMBAL_ACTR_YAW_IDX] + i, 0.0f);
  }

  for (size_t i = 0; i < g->num_pit; i++) {
    low_pass_filter_2p_reset(g->filter_out[GIMBAL_ACTR_PIT_IDX] + i, 0.0f);
  }

  ahrs_set_eulr(&(g->setpoint.eulr),
                &(g->feedback.eulr.imu)); /* 切换模式后重置设定值 */
  if (g->mode == GIMBAL_MODE_RELAX) {
    if (mode == GIMBAL_MODE_ABSOLUTE) {
      g->setpoint.eulr.yaw = g->feedback.eulr.imu.yaw;
    } else if (mode == GIMBAL_MODE_RELATIVE) {
      g->setpoint.eulr.yaw = g->feedback.eulr.encoder.yaw;
    }
  }

  if (mode == GIMBAL_MODE_SCAN) {
    g->scan_pit_direction = (rand() % 2) ? -1 : 1;
    g->scan_yaw_direction = (rand() % 2) ? -1 : 1;
  }

  g->mode = mode;
}

/**
 * @brief 初始化云台
 *
 * @param g 包含云台数据的结构体
 * @param param 包含云台参数的结构体指针
 * @param target_freq 线程预期的运行频率
 */
void gimbal_init(gimbal_t *g, const gimbal_params_t *param, float limit_max,
                 eulr_t *mech_zero, float target_freq) {
  ASSERT(g);
  ASSERT(param);

  g->param = param;            /* 初始化参数 */
  g->mode = GIMBAL_MODE_RELAX; /* 设置默认模式 */
  g->mech_zero = mech_zero;

  switch (g->param->type) {
    case GIMBAL_TYPE_ROTATRY:
      g->num_yaw = 1;
      g->num_pit = 1;
      break;
    case GIMBAL_TYPE_LINEAR:
      g->num_yaw = 1;
      g->num_pit = 2;
      break;
  }

  g->pid[GIMBAL_CTRL_YAW_OMEGA_IDX] = pvPortMalloc(sizeof(kpid_t) * g->num_yaw);
  g->pid[GIMBAL_CTRL_PIT_OMEGA_IDX] = pvPortMalloc(sizeof(kpid_t) * g->num_pit);
  g->pid[GIMBAL_CTRL_YAW_ANGLE_IDX] = pvPortMalloc(sizeof(kpid_t) * g->num_yaw);
  g->pid[GIMBAL_CTRL_PIT_ANGLE_IDX] = pvPortMalloc(sizeof(kpid_t) * g->num_pit);

  g->filter_out[GIMBAL_ACTR_YAW_IDX] =
      pvPortMalloc(sizeof(low_pass_filter_2p_t) * g->num_yaw);
  g->filter_out[GIMBAL_ACTR_PIT_IDX] =
      pvPortMalloc(sizeof(low_pass_filter_2p_t) * g->num_pit);

  g->out[GIMBAL_ACTR_YAW_IDX] = pvPortMalloc(sizeof(float) * g->num_yaw);
  g->out[GIMBAL_ACTR_PIT_IDX] = pvPortMalloc(sizeof(float) * g->num_pit);

  /* 设置软件限位 */
  if (g->param->reverse.pit) circle_reverse(&limit_max);
  g->limit.min = g->limit.max = limit_max;
  circle_add(&(g->limit.min), -g->param->pitch_travel_rad, M_2PI);

  /* 初始化云台电机控制PID和LPF */
  for (size_t i = 0; i < g->num_yaw; i++) {
    kpid_init(g->pid[GIMBAL_CTRL_YAW_ANGLE_IDX + i], KPID_MODE_NO_D,
              KPID_MODE_K_SET, target_freq,
              g->param->pid + GIMBAL_CTRL_YAW_ANGLE_IDX);
    kpid_init(g->pid[GIMBAL_CTRL_YAW_OMEGA_IDX] + i, KPID_MODE_CALC_D,
              KPID_MODE_K_DYNAMIC, target_freq,
              g->param->pid + GIMBAL_CTRL_YAW_OMEGA_IDX);
  }

  for (size_t i = 0; i < g->num_pit; i++) {
    kpid_init(g->pid[GIMBAL_CTRL_PIT_ANGLE_IDX] + i, KPID_MODE_NO_D,
              KPID_MODE_K_SET, target_freq,
              g->param->pid + GIMBAL_CTRL_PIT_ANGLE_IDX);
    kpid_init(g->pid[GIMBAL_CTRL_PIT_OMEGA_IDX] + i, KPID_MODE_CALC_D,
              KPID_MODE_K_SET, target_freq,
              g->param->pid + GIMBAL_CTRL_PIT_OMEGA_IDX);
  }

  for (size_t i = 0; i < g->num_pit; i++) {
    low_pass_filter_2p_init(g->filter_out[GIMBAL_ACTR_PIT_IDX] + i, target_freq,
                            g->param->low_pass_cutoff_freq.out);
  }

  for (size_t i = 0; i < g->num_yaw; i++) {
    low_pass_filter_2p_init(g->filter_out[GIMBAL_ACTR_YAW_IDX] + i, target_freq,
                            g->param->low_pass_cutoff_freq.out);
  }
}

/**
 * @brief 通过CAN设备更新云台反馈信息
 *
 * @param g 云台
 * @param can CAN设备
 */
void gimbal_update_feedback(gimbal_t *g,
                            const motor_feedback_group_t *gimbal_motor_yaw,
                            const motor_feedback_group_t *gimbal_motor_pit) {
  ASSERT(g);
  ASSERT(gimbal_motor_yaw);
  ASSERT(gimbal_motor_pit);

  if (g->param->type == GIMBAL_TYPE_ROTATRY)

  {
    g->feedback.eulr.encoder.yaw =
        gimbal_motor_yaw->as_array[0].rotor_abs_angle;
    g->feedback.eulr.encoder.pit =
        gimbal_motor_pit->as_array[0].rotor_abs_angle;

    if (g->param->reverse.yaw) circle_reverse(&(g->feedback.eulr.encoder.yaw));
    if (g->param->reverse.pit) circle_reverse(&(g->feedback.eulr.encoder.pit));
  }
}

/**
 * @brief 运行云台控制逻辑
 *
 * @param g 包含云台数据的结构体
 * @param fb 云台反馈信息
 * @param g_cmd 云台控制指令
 * @param dt_sec 两次调用的时间间隔
 */
void gimbal_control(gimbal_t *g, cmd_gimbal_t *g_cmd, uint32_t now) {
  ASSERT(g);
  ASSERT(g_cmd);

  g->dt = (float)(now - g->lask_wakeup) / 1000.0f;
  g->lask_wakeup = now;

  gimbal_set_mode(g, g_cmd->mode);

  /* yaw坐标正方向与遥控器操作逻辑相反 */
  g_cmd->delta_eulr.pit = g_cmd->delta_eulr.pit;
  g_cmd->delta_eulr.yaw = -g_cmd->delta_eulr.yaw;

  switch (g->mode) {
    case GIMBAL_MODE_SCAN: {
      /* 判断YAW轴运动方向 */
      const float yaw_offset = circle_error(g->feedback.eulr.imu.yaw, 0, M_2PI);

      if (fabs(yaw_offset) > ANGLE2RANDIAN(GIM_SCAN_ANGLE_BASE)) {
        if (yaw_offset > 0)
          g->scan_yaw_direction = -1;
        else {
          g->scan_yaw_direction = 1;
        }
      }

      /* 判断PIT轴运动方向 */
      if (circle_error(g->feedback.eulr.encoder.pit, g->limit.min, M_2PI) <=
          ANGLE2RANDIAN(GIM_SCAN_CRITICAL_ERR)) {
        g->scan_pit_direction = 1;
      } else if (circle_error(g->limit.max, g->feedback.eulr.encoder.pit,
                              M_2PI) <= ANGLE2RANDIAN(GIM_SCAN_CRITICAL_ERR)) {
        g->scan_pit_direction = -1;
      }

      /* 覆盖控制命令 */
      g_cmd->delta_eulr.yaw =
          SPEED2DELTA(GIM_SCAN_YAW_SPEED, g->dt) * g->scan_yaw_direction;
      g_cmd->delta_eulr.pit =
          SPEED2DELTA(GIM_SCAN_PIT_SPEED, g->dt) * g->scan_pit_direction;

      break;
    }
    default:
      break;
  }

  /* 处理yaw控制命令 */
  circle_add(&(g->setpoint.eulr.yaw), g_cmd->delta_eulr.yaw, M_2PI);

  /* 处理pitch控制命令，软件限位 */
  const float delta_max =
      circle_error(g->limit.max,
                   (g->feedback.eulr.encoder.pit + g->setpoint.eulr.pit -
                    g->feedback.eulr.imu.pit),
                   M_2PI);
  const float delta_min =
      circle_error(g->limit.min,
                   (g->feedback.eulr.encoder.pit + g->setpoint.eulr.pit -
                    g->feedback.eulr.imu.pit),
                   M_2PI);
  clampf(&(g_cmd->delta_eulr.pit), delta_min, delta_max);
  g->setpoint.eulr.pit += g_cmd->delta_eulr.pit;

  /* 重置输入指令，防止重复处理 */
  ahrs_reset_eulr(&(g_cmd->delta_eulr));

  /* 控制相关逻辑 */
  float yaw_omega_set_point, pit_omega_set_point;
  switch (g->mode) {
    case GIMBAL_MODE_RELAX:
      for (size_t i = 0; i < g->num_yaw; i++)
        g->out[GIMBAL_ACTR_YAW_IDX][i] = 0.0f;
      for (size_t i = 0; i < g->num_pit; i++)
        g->out[GIMBAL_ACTR_PIT_IDX][i] = 0.0f;

      break;
    case GIMBAL_MODE_SCAN:
    case GIMBAL_MODE_ABSOLUTE:
      for (size_t i = 0; i < g->num_yaw; i++) {
        /* Yaw轴角速度环参数计算 */
        kpid_set_k(g->pid[GIMBAL_CTRL_YAW_OMEGA_IDX] + i,
                   cf_get_value(&(g->param->st), g->feedback.eulr.imu.pit));

        /* Yaw轴角度 反馈控制 */
        yaw_omega_set_point = kpid_calc(g->pid[GIMBAL_CTRL_YAW_ANGLE_IDX] + i,
                                        g->setpoint.eulr.yaw,
                                        g->feedback.eulr.imu.yaw, 0.0f, g->dt);

        /* Yaw轴角速度 反馈控制 */
        g->out[GIMBAL_ACTR_YAW_IDX][i] =
            kpid_calc(g->pid[GIMBAL_CTRL_YAW_OMEGA_IDX] + i,
                      yaw_omega_set_point, g->feedback.gyro.z, 0.f, g->dt);
      }

      for (size_t i = 0; i < g->num_pit; i++) {
        /* Pitch轴角度 反馈控制 */
        pit_omega_set_point = kpid_calc(g->pid[GIMBAL_CTRL_PIT_ANGLE_IDX] + i,
                                        g->setpoint.eulr.pit,
                                        g->feedback.eulr.imu.pit, 0.0f, g->dt);

        /* Pitch轴角速度 反馈控制 */
        g->out[GIMBAL_ACTR_PIT_IDX][i] =
            kpid_calc(g->pid[GIMBAL_CTRL_PIT_OMEGA_IDX] + i,
                      pit_omega_set_point, g->feedback.gyro.x, 0.f, g->dt);

        /* Pitch前馈控制 */
        g->out[GIMBAL_ACTR_PIT_IDX][i] +=
            cf_get_value(&(g->param->ff), g->feedback.eulr.imu.pit);

        clampf(g->out[GIMBAL_ACTR_PIT_IDX] + i, -1.0, 1.0);
      }
      break;

    case GIMBAL_MODE_RELATIVE:
      for (size_t i = 0; i < g->num_yaw; i++)
        g->out[GIMBAL_ACTR_YAW_IDX][i] = 0.0f;
      for (size_t i = 0; i < g->num_pit; i++)
        g->out[GIMBAL_ACTR_PIT_IDX][i] = 0.0f;
      break;
  }

  /* 输出滤波 */
  for (size_t i = 0; i < g->num_yaw; i++)
    g->out[GIMBAL_ACTR_YAW_IDX][i] = low_pass_filter_2p_apply(
        g->filter_out[GIMBAL_ACTR_YAW_IDX] + i, g->out[GIMBAL_ACTR_YAW_IDX][i]);

  for (size_t i = 0; i < g->num_pit; i++)
    g->out[GIMBAL_ACTR_PIT_IDX][i] = low_pass_filter_2p_apply(
        g->filter_out[GIMBAL_ACTR_PIT_IDX] + i, g->out[GIMBAL_ACTR_PIT_IDX][i]);

  /* 处理电机反装 */
  if (g->param->reverse.yaw)
    for (size_t i = 0; i < g->num_yaw; i++)
      g->out[GIMBAL_ACTR_YAW_IDX][i] = -g->out[GIMBAL_ACTR_YAW_IDX][i];
  if (g->param->reverse.pit)
    for (size_t i = 0; i < g->num_pit; i++)
      g->out[GIMBAL_ACTR_PIT_IDX][i] = -g->out[GIMBAL_ACTR_PIT_IDX][i];
}

/**
 * @brief 复制云台输出值
 *
 * @param g 包含云台数据的结构体
 * @param out CAN设备云台输出结构体
 */
void gimbal_pack_output(gimbal_t *g, motor_control_t *pit_out,
                        motor_control_t *yaw_out) {
  ASSERT(g);
  ASSERT(pit_out);
  ASSERT(yaw_out);

  for (size_t i = 0; i < g->num_yaw; i++)
    yaw_out->as_array[i] = g->out[GIMBAL_ACTR_YAW_IDX][i];

  for (size_t i = 0; i < g->num_pit; i++)
    pit_out->as_array[i] = g->out[GIMBAL_ACTR_PIT_IDX][i];
}

/**
 * @brief 导出云台UI数据
 *
 * @param g 云台结构体
 * @param ui UI结构体
 */
void gimbal_pack_ui(const gimbal_t *g, ui_gimbal_t *ui) {
  ASSERT(g);
  ASSERT(ui);
  ui->mode = g->mode;
}
