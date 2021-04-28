/*
 * 发射器模组
 */

/* Includes ----------------------------------------------------------------- */
#include "launcher.h"

#include "bsp/pwm.h"
#include "component/game.h"
#include "component/limiter.h"
#include "component/user_math.h"
/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

/**
 * \brief 设置发射器模式
 *
 * \param l 包含发射器数据的结构体
 * \param mode 要设置的模式
 *
 * \return 函数运行结果
 */
static int8_t Launcher_SetMode(Launcher_t *l, Game_LauncherMode_t mode) {
  ASSERT(l);

  if (mode == l->mode) return LAUNCHER_OK;

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < 2; i++) {
    PID_Reset(l->pid.fric + i);
    LowPassFilter2p_Reset(l->filter.in.fric + i, 0.0f);
    LowPassFilter2p_Reset(l->filter.out.fric + i, 0.0f);
  }
  PID_Reset(&(l->pid.trig));
  LowPassFilter2p_Reset(&(l->filter.in.trig), 0.0f);
  LowPassFilter2p_Reset(&(l->filter.out.trig), 0.0f);

  /* 保证模式变换时拨弹盘不丢失起始位置 */
  while (fabsf(CircleError(l->setpoint.trig_angle, l->feedback.trig_angle,
                           M_2PI)) >= M_2PI / l->param->num_trig_tooth / 2.0f) {
    CircleAdd(&(l->setpoint.trig_angle), M_2PI / l->param->num_trig_tooth,
              M_2PI);
  }

  if (mode == LAUNCHER_MODE_LOADED) l->fire_ctrl.to_launch = 0;

  l->mode = mode;
  return 0;
}

/**
 * @brief 发射器热量控制
 *
 * @param l 包含发射器数据的结构体
 * @param l_ref 发射器所需裁判系统数据
 * @return int8_t 函数运行结果
 */
static int8_t Launcher_HeatLimit(Launcher_t *l, Referee_ForLauncher_t *l_ref) {
  ASSERT(l);
  ASSERT(l_ref);
  Launcher_HeatCtrl_t *hc = &(l->heat_ctrl);
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
  return 0;
}

/* Exported functions ------------------------------------------------------- */

/**
 * \brief 初始化发射器
 *
 * \param l 包含发射器数据的结构体
 * \param param 包含发射器参数的结构体指针
 * \param target_freq 任务预期的运行频率
 *
 * \return 函数运行结果
 */
int8_t Launcher_Init(Launcher_t *l, const Launcher_Params_t *param,
                     float target_freq) {
  ASSERT(l);
  ASSERT(param);

  l->param = param;              /* 初始化参数 */
  l->mode = LAUNCHER_MODE_RELAX; /* 设置默认模式 */

  for (size_t i = 0; i < 2; i++) {
    /* PI控制器初始化PID */
    PID_Init(l->pid.fric + i, KPID_MODE_NO_D, target_freq,
             &(param->fric_pid_param));

    LowPassFilter2p_Init(l->filter.in.fric + i, target_freq,
                         param->low_pass_cutoff_freq.in.fric);

    LowPassFilter2p_Init(l->filter.out.fric + i, target_freq,
                         param->low_pass_cutoff_freq.out.fric);
  }

  PID_Init(&(l->pid.trig), KPID_MODE_CALC_D, target_freq,
           &(param->trig_pid_param));

  LowPassFilter2p_Init(&(l->filter.in.trig), target_freq,
                       param->low_pass_cutoff_freq.in.trig);
  LowPassFilter2p_Init(&(l->filter.out.trig), target_freq,
                       param->low_pass_cutoff_freq.out.trig);

  BSP_PWM_Start(BSP_PWM_LAUNCHER_SERVO);
  BSP_PWM_Set(BSP_PWM_LAUNCHER_SERVO, param->cover_close_duty);
  return 0;
}

/**
 * \brief 更新发射器的反馈信息
 *
 * \param l 包含发射器数据的结构体
 * \param can CAN设备结构体
 *
 * \return 函数运行结果
 */
int8_t Launcher_UpdateFeedback(Launcher_t *l, const CAN_t *can) {
  ASSERT(l);
  ASSERT(can);

  for (size_t i = 0; i < 2; i++) {
    l->feedback.fric_rpm[i] = can->motor.launcher.as_array[i].rotor_speed;
  }

  /* 更新拨弹电机 */
  const float last_trig_motor_angle = l->feedback.trig_motor_angle;
  l->feedback.trig_motor_angle = can->motor.launcher.named.trig.rotor_angle;
  const float delta_motor_angle =
      CircleError(l->feedback.trig_motor_angle, last_trig_motor_angle, M_2PI);
  CircleAdd(&(l->feedback.trig_angle),
            delta_motor_angle / l->param->trig_gear_ratio, M_2PI);

  return 0;
}

/**
 * @brief 运行发射器控制逻辑
 * @warning 鼠标点击过快（100ms内多次点击）会导致热量控制被忽略
 *
 * @param l 包含发射器数据的结构体
 * @param l_cmd 发射器控制指令
 * @param l_ref 发射器使用的裁判系统数据
 * @param now 现在时刻
 * @return int8_t
 */
int8_t Launcher_Control(Launcher_t *l, CMD_LauncherCmd_t *l_cmd,
                        Referee_ForLauncher_t *l_ref, uint32_t now) {
  ASSERT(l);
  ASSERT(l_cmd);
  ASSERT(l_ref);

  l->dt = (float)(now - l->lask_wakeup) / 1000.0f;
  l->lask_wakeup = now;

  Launcher_SetMode(l, l_cmd->mode); /* 设置发射器模式 */
  Launcher_HeatLimit(l, l_ref);     /* 热量控制 */

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
      float launch_freq = HeatLimit_LauncherFreq(
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
      CalculateRpm(l->fire_ctrl.bullet_speed, l->param->fric_radius,
                   (l->param->model == LAUNCHER_MODEL_17MM));
  l->setpoint.fric_rpm[0] = -l->setpoint.fric_rpm[1];

  /* 计算拨弹电机位置的目标值 */
  if ((now - l->fire_ctrl.last_launch) >= l->fire_ctrl.launch_delay) {
    /* 将拨弹电机角度进行循环加法，每次加(减)射出一颗弹丸的弧度变化 */
    if (l_cmd->reverse_trig) { /* 反转拨盘，用来解决卡顿*/
      CircleAdd(&(l->setpoint.trig_angle), M_2PI / l->param->num_trig_tooth,
                M_2PI);
    } else {
      CircleAdd(&(l->setpoint.trig_angle), -M_2PI / l->param->num_trig_tooth,
                M_2PI);
      /* 计算已发射弹丸 */
      l->fire_ctrl.launched++;
      l->fire_ctrl.last_launch = now;
    }
  }

  switch (l->mode) {
    case LAUNCHER_MODE_RELAX:
      for (size_t i = 0; i < LAUNCHER_ACTR_NUM; i++) {
        l->out[i] = 0.0f;
      }
      BSP_PWM_Stop(BSP_PWM_LAUNCHER_SERVO);
      break;

    case LAUNCHER_MODE_SAFE:
    case LAUNCHER_MODE_LOADED:
      /* 控制拨弹电机 */
      l->feedback.trig_angle =
          LowPassFilter2p_Apply(&(l->filter.in.trig), l->feedback.trig_angle);

      l->out[LAUNCHER_ACTR_TRIG_IDX] =
          PID_Calc(&(l->pid.trig), l->setpoint.trig_angle,
                   l->feedback.trig_angle, 0.0f, l->dt);
      l->out[LAUNCHER_ACTR_TRIG_IDX] = LowPassFilter2p_Apply(
          &(l->filter.out.trig), l->out[LAUNCHER_ACTR_TRIG_IDX]);

      for (size_t i = 0; i < 2; i++) {
        /* 控制摩擦轮 */
        l->feedback.fric_rpm[i] = LowPassFilter2p_Apply(
            &(l->filter.in.fric[i]), l->feedback.fric_rpm[i]);

        l->out[LAUNCHER_ACTR_FRIC1_IDX + i] =
            PID_Calc(&(l->pid.fric[i]), l->setpoint.fric_rpm[i],
                     l->feedback.fric_rpm[i], 0.0f, l->dt);

        l->out[LAUNCHER_ACTR_FRIC1_IDX + i] = LowPassFilter2p_Apply(
            &(l->filter.out.fric[i]), l->out[LAUNCHER_ACTR_FRIC1_IDX + i]);
      }

      /* 根据弹仓盖开关状态更新弹舱盖打开时舵机PWM占空比 */
      if (l_cmd->cover_open) {
        BSP_PWM_Start(BSP_PWM_LAUNCHER_SERVO);
        BSP_PWM_Set(BSP_PWM_LAUNCHER_SERVO, l->param->cover_open_duty);
      } else {
        BSP_PWM_Start(BSP_PWM_LAUNCHER_SERVO);
        BSP_PWM_Set(BSP_PWM_LAUNCHER_SERVO, l->param->cover_close_duty);
      }
      break;
  }
  return 0;
}

/**
 * \brief 复制发射器输出值
 *
 * \param l 包含发射器数据的结构体
 * \param out CAN设备发射器输出结构体
 */
void Launcher_PackOutput(Launcher_t *l, CAN_LauncherOutput_t *out) {
  ASSERT(l);
  ASSERT(out);
  for (size_t i = 0; i < LAUNCHER_ACTR_NUM; i++) {
    out->as_array[i] = l->out[i];
  }
}

/**
 * \brief 清空输出值
 *
 * \param output 要清空的结构体
 */
void Launcher_ResetOutput(CAN_LauncherOutput_t *output) {
  ASSERT(output);
  memset(output, 0, sizeof(*output));
}

/**
 * @brief 导出发射器UI数据
 *
 * @param l 发射器结构体
 * @param ui UI结构体
 */
void Launcher_PackUi(Launcher_t *l, Referee_LauncherUI_t *ui) {
  ASSERT(l);
  ASSERT(ui);
  ui->mode = l->mode;
  ui->fire = l->fire_ctrl.fire_mode;
}
