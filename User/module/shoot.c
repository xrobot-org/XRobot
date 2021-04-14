/*
 * 发射器模组
 */

/* Includes ----------------------------------------------------------------- */
#include "shoot.h"

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
 * \param c 包含发射器数据的结构体
 * \param mode 要设置的模式
 *
 * \return 函数运行结果
 */
static int8_t Shoot_SetMode(Shoot_t *s, CMD_ShootMode_t mode) {
  if (s == NULL) return -1;

  if (mode == s->mode) return SHOOT_OK;

  /* 切换模式后重置PID和滤波器 */
  for (uint8_t i = 0; i < 2; i++) {
    PID_Reset(s->pid.fric + i);
    LowPassFilter2p_Reset(s->filter.in.fric + i, 0.0f);
    LowPassFilter2p_Reset(s->filter.out.fric + i, 0.0f);
  }
  PID_Reset(&(s->pid.trig));
  LowPassFilter2p_Reset(&(s->filter.in.trig), 0.0f);
  LowPassFilter2p_Reset(&(s->filter.out.trig), 0.0f);

  while (fabsf(CircleError(s->setpoint.trig_angle, s->feedback.trig_angle,
                           M_2PI)) >= M_2PI / s->param->num_trig_tooth / 2.0f) {
    CircleAdd(&(s->setpoint.trig_angle), M_2PI / s->param->num_trig_tooth,
              M_2PI);
  }

  if (mode == SHOOT_MODE_LOADED) s->fire_ctrl.to_shoot = 0;

  s->mode = mode;
  return 0;
}

/**
 * @brief
 *
 * @param s
 * @param s_ref
 * @return int8_t
 */
static int8_t Shoot_HeatLimit(Shoot_t *s, Referee_ForShoot_t *s_ref) {
  Shoot_HeatCtrl_t *hc = &(s->heat_ctrl);
  /* 当裁判系统在线时启用热量控制与射速控制 */
  if (s_ref->ref_status == REF_STATUS_RUNNING) {
    /* 根据机器人型号获得对应数据 */
    if (s->param->model == SHOOT_MODEL_42MM) {
      hc->heat = s_ref->power_heat.shoot_42_heat;
      hc->heat_limit = s_ref->robot_status.shoot_42_heat_limit;
      hc->speed_limit = s_ref->robot_status.shoot_42_speed_limit;
      hc->cooling_rate = s_ref->robot_status.shoot_42_cooling_rate;
      hc->heat_increase = GAME_HEAT_INCREASE_42MM;
    } else if (s->param->model == SHOOT_MODEL_17MM) {
      hc->heat = s_ref->power_heat.shoot_id1_17_heat;
      hc->heat_limit = s_ref->robot_status.shoot_id1_17_heat_limit;
      hc->speed_limit = s_ref->robot_status.shoot_id1_17_speed_limit;
      hc->cooling_rate = s_ref->robot_status.shoot_id1_17_cooling_rate;
      hc->heat_increase = GAME_HEAT_INCREASE_17MM;
    }
    /* 检测热量更新后,计算可发射弹丸 */
    if ((hc->heat != hc->last_heat) || (hc->heat == 0)) {
      hc->available_shot =
          (uint32_t)floorf((hc->heat_limit - hc->heat) / hc->heat_increase);
      hc->last_heat = hc->heat;
    }
    /* 计算已发射弹丸 */
    if (s_ref->shoot_data.bullet_speed != hc->last_bullet_speed) {
      hc->last_bullet_speed = s_ref->shoot_data.bullet_speed;
    }
    s->fire_ctrl.bullet_speed = hc->speed_limit;
  } else {
    /* 裁判系统离线，不启用热量控制 */
    hc->available_shot = 10;
    s->fire_ctrl.bullet_speed = s->param->bullet_speed;
  }
  return 0;
}

/* Exported functions ------------------------------------------------------- */

/**
 * \brief 初始化发射器
 *
 * \param s 包含发射器数据的结构体
 * \param param 包含发射器参数的结构体指针
 * \param target_freq 任务预期的运行频率
 *
 * \return 函数运行结果
 */
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *param, float target_freq) {
  if (s == NULL) return -1;

  s->param = param;           /* 初始化参数 */
  s->mode = SHOOT_MODE_RELAX; /* 设置默认模式 */

  for (uint8_t i = 0; i < 2; i++) {
    /* PI控制器初始化PID */
    PID_Init(s->pid.fric + i, KPID_MODE_NO_D, target_freq,
             &(param->fric_pid_param));

    LowPassFilter2p_Init(s->filter.in.fric + i, target_freq,
                         param->low_pass_cutoff_freq.in.fric);

    LowPassFilter2p_Init(s->filter.out.fric + i, target_freq,
                         param->low_pass_cutoff_freq.out.fric);
  }

  PID_Init(&(s->pid.trig), KPID_MODE_CALC_D, target_freq,
           &(param->trig_pid_param));

  LowPassFilter2p_Init(&(s->filter.in.trig), target_freq,
                       param->low_pass_cutoff_freq.in.trig);
  LowPassFilter2p_Init(&(s->filter.out.trig), target_freq,
                       param->low_pass_cutoff_freq.out.trig);

  BSP_PWM_Start(BSP_PWM_SHOOT_SERVO);
  BSP_PWM_Set(BSP_PWM_SHOOT_SERVO, param->cover_close_duty);
  return 0;
}

/**
 * \brief 更新发射器的反馈信息
 *
 * \param s 包含发射器数据的结构体
 * \param can CAN设备结构体
 *
 * \return 函数运行结果
 */
int8_t Shoot_UpdateFeedback(Shoot_t *s, const CAN_t *can) {
  if (s == NULL) return -1;
  if (can == NULL) return -1;

  for (uint8_t i = 0; i < 2; i++) {
    s->feedback.fric_rpm[i] = can->motor.shoot.as_array[i].rotor_speed;
  }

  /* 更新拨弹电机 */
  float last_trig_motor_angle = s->feedback.trig_motor_angle;
  s->feedback.trig_motor_angle = can->motor.shoot.named.trig.rotor_angle;
  float motor_angle_delta =
      CircleError(s->feedback.trig_motor_angle, last_trig_motor_angle, M_2PI);
  CircleAdd(&(s->feedback.trig_angle),
            motor_angle_delta / s->param->trig_gear_ratio, M_2PI);

  return 0;
}

/**
 * @brief 运行发射器控制逻辑
 *
 * @param s 包含发射器数据的结构体
 * @param s_cmd 发射器控制指令
 * @param s_ref 发射器使用的裁判系统数据
 * @param now 现在时刻
 * @return int8_t
 */
int8_t Shoot_Control(Shoot_t *s, CMD_ShootCmd_t *s_cmd,
                     Referee_ForShoot_t *s_ref, uint32_t now) {
  if (s == NULL) return -1;

  s->dt = (float)(now - s->lask_wakeup) / 1000.0f;
  s->lask_wakeup = now;

  Shoot_SetMode(s, s_cmd->mode); /* 设置发射器模式 */
  Shoot_HeatLimit(s, s_ref);     /* 热量控制 */
  /* 根据开火模式计算发射行为 */
  s->fire_ctrl.fire_mode = s_cmd->fire_mode;
  int32_t max_burst;
  switch (s_cmd->fire_mode) {
    case FIRE_MODE_SINGLE: /* 点射开火模式 */
      max_burst = 1;
      break;
    case FIRE_MODE_BURST: /* 连发开火模式 */
      max_burst = 5;
      break;
    default:
      break;
  }

  switch (s_cmd->fire_mode) {
    case FIRE_MODE_SINGLE:  /* 点射开火模式 */
    case FIRE_MODE_BURST: { /* 连发开火模式 */
      s->fire_ctrl.first_fire = s_cmd->fire && !s->fire_ctrl.last_fire;
      s->fire_ctrl.last_fire = s_cmd->fire;
      s_cmd->fire = s->fire_ctrl.first_fire;
      int32_t max_shot = s->heat_ctrl.available_shot - s->fire_ctrl.shooted;
      if (s_cmd->fire && !s->fire_ctrl.to_shoot) {
        s->fire_ctrl.to_shoot = min(max_burst, max_shot);
      }
      if (s->fire_ctrl.shooted >= s->fire_ctrl.to_shoot) {
        s_cmd->fire = false;
        s->fire_ctrl.period_ms = UINT32_MAX;
        s->fire_ctrl.shooted = 0;
        s->fire_ctrl.to_shoot = 0;
      } else {
        s_cmd->fire = true;
        s->fire_ctrl.period_ms = s->param->min_shoot_delay;
      }
      break;
    }
    case FIRE_MODE_CONT: { /* 持续开火模式 */
      float shoot_freq = HeatLimit_ShootFreq(
          s->heat_ctrl.heat, s->heat_ctrl.heat_limit, s->heat_ctrl.cooling_rate,
          s->heat_ctrl.heat_increase, s->param->model == SHOOT_MODEL_42MM);
      s->fire_ctrl.period_ms =
          (shoot_freq == 0.0f) ? UINT32_MAX : (uint32_t)(1000.f / shoot_freq);
      break;
    }
    default:
      break;
  }

  /* 根据模式选择是否使用计算出来的值 */
  switch (s->mode) {
    case SHOOT_MODE_RELAX:
    case SHOOT_MODE_SAFE:
      s->fire_ctrl.bullet_speed = 0.0f;
      s->fire_ctrl.period_ms = UINT32_MAX;
    case SHOOT_MODE_LOADED:
      break;
  }

  /* 计算摩擦轮转速的目标值 */
  s->setpoint.fric_rpm[1] =
      CalculateRpm(s->fire_ctrl.bullet_speed, s->param->fric_radius,
                   (s->param->model == SHOOT_MODEL_17MM));
  s->setpoint.fric_rpm[0] = -s->setpoint.fric_rpm[1];

  /* 计算拨弹电机位置的目标值 */
  if (((now - s->fire_ctrl.last_shoot) >= s->fire_ctrl.period_ms) &&
      (s_cmd->fire)) {
    /* 将拨弹电机角度进行循环加法，每次加(减)射出一颗弹丸的弧度变化 */
    if (s_cmd->reverse_trig) { /* 反转拨弹 */
      CircleAdd(&(s->setpoint.trig_angle), M_2PI / s->param->num_trig_tooth,
                M_2PI);
    } else {
      CircleAdd(&(s->setpoint.trig_angle), -M_2PI / s->param->num_trig_tooth,
                M_2PI);
      s->fire_ctrl.shooted++;
      s->fire_ctrl.last_shoot = now;
    }
  }

  switch (s->mode) {
    case SHOOT_MODE_RELAX:
      for (uint8_t i = 0; i < SHOOT_ACTR_NUM; i++) {
        s->out[i] = 0.0f;
      }
      BSP_PWM_Stop(BSP_PWM_SHOOT_SERVO);
      break;

    case SHOOT_MODE_SAFE:
    case SHOOT_MODE_LOADED:
      /* 控制拨弹电机 */
      s->feedback.trig_angle =
          LowPassFilter2p_Apply(&(s->filter.in.trig), s->feedback.trig_angle);

      s->out[SHOOT_ACTR_TRIG_IDX] =
          PID_Calc(&(s->pid.trig), s->setpoint.trig_angle,
                   s->feedback.trig_angle, 0.0f, s->dt);
      s->out[SHOOT_ACTR_TRIG_IDX] = LowPassFilter2p_Apply(
          &(s->filter.out.trig), s->out[SHOOT_ACTR_TRIG_IDX]);

      for (uint8_t i = 0; i < 2; i++) {
        /* 控制摩擦轮 */
        s->feedback.fric_rpm[i] = LowPassFilter2p_Apply(
            &(s->filter.in.fric[i]), s->feedback.fric_rpm[i]);

        s->out[SHOOT_ACTR_FRIC1_IDX + i] =
            PID_Calc(&(s->pid.fric[i]), s->setpoint.fric_rpm[i],
                     s->feedback.fric_rpm[i], 0.0f, s->dt);

        s->out[SHOOT_ACTR_FRIC1_IDX + i] = LowPassFilter2p_Apply(
            &(s->filter.out.fric[i]), s->out[SHOOT_ACTR_FRIC1_IDX + i]);
      }

      /* 根据弹仓盖开关状态更新弹舱盖打开时舵机PWM占空比 */
      if (s_cmd->cover_open) {
        BSP_PWM_Start(BSP_PWM_SHOOT_SERVO);
        BSP_PWM_Set(BSP_PWM_SHOOT_SERVO, s->param->cover_open_duty);
      } else {
        BSP_PWM_Start(BSP_PWM_SHOOT_SERVO);
        BSP_PWM_Set(BSP_PWM_SHOOT_SERVO, s->param->cover_close_duty);
      }
      break;
  }
  return 0;
}

/**
 * \brief 复制发射器输出值
 *
 * \param s 包含发射器数据的结构体
 * \param out CAN设备发射器输出结构体
 */
void Shoot_DumpOutput(Shoot_t *s, CAN_ShootOutput_t *out) {
  for (uint8_t i = 0; i < SHOOT_ACTR_NUM; i++) {
    out->as_array[i] = s->out[i];
  }
}

/**
 * \brief 清空输出值
 *
 * \param output 要清空的结构体
 */
void Shoot_ResetOutput(CAN_ShootOutput_t *output) {
  int i = 0;
  for (i = 0; i < 3; i++) {
    output->as_array[i] = 0.0f;
  }
}

/**
 * @brief 导出发射器UI数据
 *
 * @param s 发射器结构体
 * @param ui UI结构体
 */
void Shoot_DumpUI(Shoot_t *s, Referee_ShootUI_t *ui) {
  ui->mode = s->mode;
  ui->fire = s->fire_ctrl.fire_mode;
}
