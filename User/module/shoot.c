/*
 * 射击模组
 */

/* Includes ----------------------------------------------------------------- */
#include "shoot.h"

#include "bsp/pwm.h"
#include "component\limiter.h"
/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */

#define HEAT_INCREASE_42MM (100.f) /* 每发射一颗42mm弹丸增加100热量 */
#define HEAT_INCREASE_17MM (10.f)  /* 每发射一颗17mm弹丸增加10热量 */

/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

/**
 * \brief 设置射击模式
 *
 * \param c 包含射击数据的结构体
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

  s->setpoint.trig_angle = s->feedback.trig_angle;

  s->mode = mode;
  return 0;
}

/* Exported functions ------------------------------------------------------- */

/**
 * \brief 初始化射击
 *
 * \param s 包含射击数据的结构体
 * \param param 包含射击参数的结构体指针
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
 * \brief 更新射击的反馈信息
 *
 * \param s 包含射击数据的结构体
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
  CircleAdd(&(s->feedback.trig_angle), motor_angle_delta / 36.0f, M_2PI);
  return 0;
}

/**
 * \brief 运行射击控制逻辑
 *
 * \param s 包含射击数据的结构体
 * \param s_cmd 射击控制指令
 * \param dt_sec 两次调用的时间间隔
 *
 * \return 函数运行结果
 */
int8_t Shoot_Control(Shoot_t *s, CMD_ShootCmd_t *s_cmd,
                     Referee_ForShoot_t *s_ref, uint32_t now) {
  if (s == NULL) return -1;

  s->dt = (float)(now - s->lask_wakeup) / 1000.0f;
  s->lask_wakeup = now;

  Shoot_SetMode(s, s_cmd->mode); /* 设置射击模式 */

  float shoot_freq;   /* 射击频率 */
  float bullet_speed; /* 弹丸初速 */
  /* 当裁判系统在线时启用热量控制与射速控制 */
  if (s_ref->ref_status == REF_STATUS_RUNNING) {
    float heat, heat_limit, speed_limit, cooling_rate;
    uint32_t available_shot; /* 热量范围内还可以发射的数量 */

    if (s_ref->robot_status.robot_id == 1 ||
        s_ref->robot_status.robot_id == 101) { /* 英雄 */
      heat = s_ref->power_heat.shoot_42_heat;
      heat_limit = s_ref->robot_status.shoot_42_heat_limit;
      speed_limit = s_ref->robot_status.shoot_42_speed_limit;
      cooling_rate = s_ref->robot_status.shoot_42_cooling_rate;

    } else { /* 其他 */
      heat = s_ref->power_heat.shoot_id1_17_heat;
      heat_limit = s_ref->robot_status.shoot_id1_17_heat_limit;
      speed_limit = s_ref->robot_status.shoot_id1_17_speed_limit;
      cooling_rate = s_ref->robot_status.shoot_id1_17_cooling_rate;
    }
    bullet_speed = speed_limit;
    available_shot = (uint32_t)floorf((heat_limit - heat) / HEAT_INCREASE_42MM);

    // TODO shoot_freq;

    shoot_freq = HeatLimit_ShootFreq(heat / heat_limit,
                                     cooling_rate / HEAT_INCREASE_42MM,
                                     s->param->bullet_speed);
  } else {
    shoot_freq = s->param->shoot_freq;
    bullet_speed = s->param->bullet_speed;
  }

  /* 根据模式选择是否使用计算出来的值 */
  switch (s->mode) {
    case SHOOT_MODE_RELAX:
    case SHOOT_MODE_SAFE:
      bullet_speed = 0.0f;
    case SHOOT_MODE_STDBY:
      shoot_freq = 0.0f;
      break;
    case SHOOT_MODE_FIRE:
      break;
  }

  switch (s_cmd->fire) {
    case FIRE_MODE_SINGLE:
    case FIRE_MODE_BURST:
    case FIRE_MODE_CONT:
    case FIRE_MODE_NUM:
      break;
  }

  s->num_shot_bullet = 0;

  /* 计算摩擦轮转速的目标值 */
  s->setpoint.fric_rpm[1] =
      CalculateRpm(bullet_speed * 0.9f, s->param->fric_radius);
  s->setpoint.fric_rpm[0] = -s->setpoint.fric_rpm[1];

  /* 计算拨弹电机位置的目标值 */
  uint32_t period_ms =
      (shoot_freq == 0.0f) ? UINT32_MAX : (uint32_t)(1000.0f / shoot_freq);

  if ((now - s->last_shoot) >= period_ms) {
    /* 将拨弹电机角度进行循环加法，每次加(减)射出一颗子弹的弧度变化 */
    CircleAdd(&(s->setpoint.trig_angle), -M_2PI / s->param->num_trig_tooth,
              M_2PI);
    s->last_shoot = now;
  }

  switch (s->mode) {
    case SHOOT_MODE_RELAX:
      for (uint8_t i = 0; i < SHOOT_ACTR_NUM; i++) {
        s->out[i] = 0.0f;
      }
      BSP_PWM_Stop(BSP_PWM_SHOOT_SERVO);
      break;

    case SHOOT_MODE_SAFE:
    case SHOOT_MODE_STDBY:
    case SHOOT_MODE_FIRE:
      /* Filter feedback. */
      s->feedback.trig_angle =
          LowPassFilter2p_Apply(&(s->filter.in.trig), s->feedback.trig_angle);

      s->out[SHOOT_ACTR_TRIG_IDX] =
          PID_Calc(&(s->pid.trig), s->setpoint.trig_angle,
                   s->feedback.trig_angle, 0.0f, s->dt);
      s->out[SHOOT_ACTR_TRIG_IDX] = LowPassFilter2p_Apply(
          &(s->filter.out.trig), s->out[SHOOT_ACTR_TRIG_IDX]);

      for (uint8_t i = 0; i < 2; i++) {
        /* Filter feedback. */
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
        BSP_PWM_Set(BSP_PWM_SHOOT_SERVO, s->param->cover_open_duty);
      } else {
        BSP_PWM_Set(BSP_PWM_SHOOT_SERVO, s->param->cover_close_duty);
      }
      break;
  }
  return 0;
}

/**
 * \brief 复制射击输出值
 *
 * \param s 包含射击数据的结构体
 * \param out CAN设备射击输出结构体
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
