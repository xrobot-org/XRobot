/**
 * @file launcher.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 弹丸发射器模块
 * @version 1.0.0
 * @date 2021-05-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "mod_launcher.h"

#include "bsp_pwm.h"
#include "comp_game.h"
#include "comp_limiter.h"
#include "comp_utils.h"

/**
 * @brief 设置发射器模式
 *
 * @param l 包含发射器数据的结构体
 * @param mode 要设置的模式
 */
static void launcher_set_mode(launcher_t *l, launcher_mode_t mode) {
  ASSERT(l);

  if (mode == l->mode) return;

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
    kpid_reset(l->pid.fric + i);
    low_pass_filter_2p_reset(l->filter.in.fric + i, 0.0f);
    low_pass_filter_2p_reset(l->filter.out.fric + i, 0.0f);
  }
  for (int i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
    kpid_reset(l->pid.trig + i);
    low_pass_filter_2p_reset(l->filter.in.trig + i, 0.0f);
    low_pass_filter_2p_reset(l->filter.out.trig + i, 0.0f);
  }

  /* 保证模式变换时拨弹盘不丢失起始位置 */
  while (fabsf(circle_error(l->setpoint.trig_angle, l->feedback.trig_angle,
                            M_2PI)) >=
         M_2PI / l->param->num_trig_tooth / 2.0f) {
    circle_add(&(l->setpoint.trig_angle), M_2PI / l->param->num_trig_tooth,
               M_2PI);
  }

  if (mode == LAUNCHER_MODE_LOADED) l->fire_ctrl.to_launch = 0;

  l->mode = mode;
}

/**
 * @brief 发射器热量控制
 *
 * @param l 包含发射器数据的结构体
 * @param l_ref 发射器所需裁判系统数据
 */
static void launcher_heat_limit(launcher_t *l, referee_for_launcher_t *l_ref) {
  ASSERT(l);
  ASSERT(l_ref);
  launcher_heat_ctrl_t *hc = &(l->heat_ctrl);
  /* 当裁判系统在线时启用热量控制与射速控制 */
  if (l_ref->status == REF_STATUS_RUNNING) {
    /* 根据机器人型号获得对应数据 */
    if (l->param->model == LAUNCHER_MODEL_42MM) {
      hc->heat = l_ref->power_heat.launcher_42_heat;
      hc->heat_limit = l_ref->robot_status.launcher_42_heat_limit;
      hc->speed_limit = l_ref->robot_status.launcher_42_speed_limit;
      hc->cooling_rate = l_ref->robot_status.launcher_42_cooling_rate;
      hc->heat_increase = GAME_HEAT_INCREASE_42MM;
    } else if (l->param->model == LAUNCHER_MODEL_17MM) {
      hc->heat = l_ref->power_heat.launcher_id1_17_heat;
      hc->heat_limit = l_ref->robot_status.launcher_id1_17_heat_limit;
      hc->speed_limit = l_ref->robot_status.launcher_id1_17_speed_limit;
      hc->cooling_rate = l_ref->robot_status.launcher_id1_17_cooling_rate;
      hc->heat_increase = GAME_HEAT_INCREASE_17MM;
    }
    /* 检测热量更新后,计算可发射弹丸 */
    if ((hc->heat != hc->last_heat) || (hc->heat == 0)) {
      hc->available_shot =
          (uint32_t)floorf((hc->heat_limit - hc->heat) / hc->heat_increase);
      hc->last_heat = hc->heat;
    }
    l->fire_ctrl.bullet_speed = hc->speed_limit;
  } else {
    /* 裁判系统离线，不启用热量控制 */
    hc->available_shot = 10;
    l->fire_ctrl.bullet_speed = l->param->default_bullet_speed;
  }
}

/**
 * @brief 初始化发射器
 *
 * @param l 包含发射器数据的结构体
 * @param param 包含发射器参数的结构体指针
 * @param target_freq 线程预期的运行频率
 */
void launcher_init(launcher_t *l, const launcher_params_t *param,
                   float target_freq) {
  ASSERT(l);
  ASSERT(param);

  l->param = param;              /* 初始化参数 */
  l->mode = LAUNCHER_MODE_RELAX; /* 设置默认模式 */

  for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
    /* PI控制器初始化PID */
    kpid_init(l->pid.fric + i, KPID_MODE_NO_D, target_freq,
              &(param->fric_pid_param));

    low_pass_filter_2p_init(l->filter.in.fric + i, target_freq,
                            param->low_pass_cutoff_freq.in.fric);

    low_pass_filter_2p_init(l->filter.out.fric + i, target_freq,
                            param->low_pass_cutoff_freq.out.fric);
  }

  for (int i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
    kpid_init(l->pid.trig + i, KPID_MODE_CALC_D, target_freq,
              &(param->trig_pid_param));

    low_pass_filter_2p_init(l->filter.in.trig + i, target_freq,
                            param->low_pass_cutoff_freq.in.trig);
    low_pass_filter_2p_init(l->filter.out.trig + i, target_freq,
                            param->low_pass_cutoff_freq.out.trig);
  }

  bsp_pwm_start(BSP_PWM_LAUNCHER_SERVO);
  bsp_pwm_set(BSP_PWM_LAUNCHER_SERVO, param->cover_close_duty);
}

/**
 * @brief 更新发射器的反馈信息
 *
 * @param l 包含发射器数据的结构体
 * @param can CAN设备结构体
 */
void launcher_update_feedback(
    launcher_t *l, const motor_feedback_group_t *launcher_motor_trig,
    const motor_feedback_group_t *launcher_motor_fric) {
  ASSERT(l);
  ASSERT(launcher_motor_fric);
  ASSERT(launcher_motor_trig);

  for (size_t i = 0; i < 2; i++) {
    l->feedback.fric_rpm[i] = launcher_motor_fric->as_array[i].rotational_speed;
  }

  /* 更新拨弹电机 */
  const float last_trig_motor_angle = l->feedback.trig_motor_angle;
  l->feedback.trig_motor_angle =
      launcher_motor_trig->as_launcher_trig.trig.rotor_abs_angle;
  const float delta_motor_angle =
      circle_error(l->feedback.trig_motor_angle, last_trig_motor_angle, M_2PI);
  circle_add(&(l->feedback.trig_angle),
             delta_motor_angle / l->param->trig_gear_ratio, M_2PI);
}

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
                      referee_for_launcher_t *l_ref, uint32_t now) {
  ASSERT(l);
  ASSERT(l_cmd);
  ASSERT(l_ref);

  l->dt = (float)(now - l->lask_wakeup) / 1000.0f;
  l->lask_wakeup = now;

  launcher_set_mode(l, l_cmd->mode); /* 设置发射器模式 */
  launcher_heat_limit(l, l_ref);     /* 热量控制 */

  /* 根据开火模式计算发射行为 */
  l->fire_ctrl.fire_mode = l_cmd->fire_mode;
  uint32_t max_burst;
  switch (l_cmd->fire_mode) {
    case FIRE_MODE_SINGLE: /* 点射开火模式 */
      max_burst = 1;
      break;
    case FIRE_MODE_BURST: /* 爆发开火模式 */
      max_burst = 5;
      break;
    default:
      max_burst = 1;
      break;
  }

  switch (l_cmd->fire_mode) {
    case FIRE_MODE_SINGLE: /* 点射开火模式 */
    case FIRE_MODE_BURST:  /* 爆发开火模式 */

      /* 计算是否是第一次按下开火键 */
      l->fire_ctrl.first_pressed_fire = l_cmd->fire && !l->fire_ctrl.last_fire;
      l->fire_ctrl.last_fire = l_cmd->fire;

      /* 设置要发射多少弹丸 */
      if (l->fire_ctrl.first_pressed_fire && !l->fire_ctrl.to_launch) {
        l->fire_ctrl.to_launch = MIN(
            max_burst, (l->heat_ctrl.available_shot - l->fire_ctrl.launched));
      }

      /* 以下逻辑保证触发后一定会打完预设的弹丸，完成爆发 */
      if (l->fire_ctrl.launched >= l->fire_ctrl.to_launch) {
        l->fire_ctrl.launch_delay = UINT32_MAX;
        l->fire_ctrl.launched = 0;
        l->fire_ctrl.to_launch = 0;
      } else {
        l->fire_ctrl.launch_delay = l->param->min_launch_delay;
      }
      break;

    case FIRE_MODE_CONT: { /* 持续开火模式 */
      float launch_freq = limit_launcher_freq(
          l->heat_ctrl.heat, l->heat_ctrl.heat_limit, l->heat_ctrl.cooling_rate,
          l->heat_ctrl.heat_increase, l->param->model == LAUNCHER_MODEL_42MM);
      l->fire_ctrl.launch_delay =
          (launch_freq == 0.0f) ? UINT32_MAX : (uint32_t)(1000.f / launch_freq);
      break;
    }
    default:
      break;
  }

  /* 根据模式选择是否使用计算出来的值 */
  switch (l->mode) {
    case LAUNCHER_MODE_RELAX:
    case LAUNCHER_MODE_SAFE:
      l->fire_ctrl.bullet_speed = 0.0f;
      l->fire_ctrl.launch_delay = UINT32_MAX;
    case LAUNCHER_MODE_LOADED:
      break;
  }

  /* 计算摩擦轮转速的目标值 */
  l->setpoint.fric_rpm[1] =
      bullet_speed_to_fric_rpm(l->fire_ctrl.bullet_speed, l->param->fric_radius,
                               (l->param->model == LAUNCHER_MODEL_17MM));
  l->setpoint.fric_rpm[0] = -l->setpoint.fric_rpm[1];

  /* 计算拨弹电机位置的目标值 */
  if ((now - l->fire_ctrl.last_launch) >= l->fire_ctrl.launch_delay) {
    /* 将拨弹电机角度进行循环加法，每次加(减)射出一颗弹丸的弧度变化 */
    if (l_cmd->reverse_trig) { /* 反转拨盘，用来解决卡顿*/
      circle_add(&(l->setpoint.trig_angle), M_2PI / l->param->num_trig_tooth,
                 M_2PI);
    } else {
      circle_add(&(l->setpoint.trig_angle), -M_2PI / l->param->num_trig_tooth,
                 M_2PI);
      /* 计算已发射弹丸 */
      l->fire_ctrl.launched++;
      l->fire_ctrl.last_launch = now;
    }
  }

  switch (l->mode) {
    case LAUNCHER_MODE_RELAX:
      for (size_t i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
        l->trig_out[i] = 0.0f;
      }
      for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
        l->fric_out[i] = 0.0f;
      }
      bsp_pwm_stop(BSP_PWM_LAUNCHER_SERVO);
      break;

    case LAUNCHER_MODE_SAFE:
    case LAUNCHER_MODE_LOADED:
      for (int i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
        /* 控制拨弹电机 */
        l->feedback.trig_angle = low_pass_filter_2p_apply(
            l->filter.in.trig + i, l->feedback.trig_angle);

        l->trig_out[i] = kpid_calc(l->pid.trig + i, l->setpoint.trig_angle,
                                   l->feedback.trig_angle, 0.0f, l->dt);
        l->trig_out[LAUNCHER_ACTR_TRIG_IDX] = low_pass_filter_2p_apply(
            l->filter.out.trig + i, l->trig_out[LAUNCHER_ACTR_TRIG_IDX]);
      }

      for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
        /* 控制摩擦轮 */
        l->feedback.fric_rpm[i] = low_pass_filter_2p_apply(
            l->filter.in.fric + i, l->feedback.fric_rpm[i]);

        l->fric_out[LAUNCHER_ACTR_FRIC1_IDX + i] =
            kpid_calc(l->pid.fric + i, l->setpoint.fric_rpm[i],
                      l->feedback.fric_rpm[i], 0.0f, l->dt);

        l->fric_out[LAUNCHER_ACTR_FRIC1_IDX + i] = low_pass_filter_2p_apply(
            l->filter.out.fric + i, l->fric_out[LAUNCHER_ACTR_FRIC1_IDX + i]);
      }

      /* 根据弹仓盖开关状态更新弹舱盖打开时舵机PWM占空比 */
      if (l_cmd->cover_open) {
        bsp_pwm_start(BSP_PWM_LAUNCHER_SERVO);
        bsp_pwm_set(BSP_PWM_LAUNCHER_SERVO, l->param->cover_open_duty);
      } else {
        bsp_pwm_start(BSP_PWM_LAUNCHER_SERVO);
        bsp_pwm_set(BSP_PWM_LAUNCHER_SERVO, l->param->cover_close_duty);
      }
      break;
  }
}

/**
 * @brief 复制发射器输出值
 *
 * @param l 包含发射器数据的结构体
 * @param out CAN设备发射器输出结构体
 */
void launcher_pack_output(launcher_t *l, motor_control_t *trig_out,
                          motor_control_t *fric_out) {
  ASSERT(l);
  ASSERT(trig_out);
  ASSERT(fric_out);

  for (size_t i = 0; i < LAUNCHER_ACTR_FRIC_NUM; i++) {
    fric_out->as_array[i] = l->fric_out[i];
  }

  for (size_t i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
    trig_out->as_array[i] = l->trig_out[i];
  }
}

/**
 * @brief 导出发射器UI数据
 *
 * @param l 发射器结构体
 * @param ui UI结构体
 */
void launcher_pack_ui(launcher_t *l, ui_launcher_t *ui) {
  ASSERT(l);
  ASSERT(ui);
  ui->mode = l->mode;
  ui->fire = l->fire_ctrl.fire_mode;
}
