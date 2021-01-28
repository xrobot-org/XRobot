/*
 * 射击模组
 */

/* Includes ----------------------------------------------------------------- */
#include "shoot.h"

#include "component\limiter.h"
/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

/**
 * \brief 控制扳机的回调函数
 *
 * \param arg 参数，这里输入Shoot_t
 *
 * \return 函数运行结果
 */
static void TrigTimerCallback(void *arg) {
  Shoot_t *s = (Shoot_t *)arg;
  CircleAdd(&(s->setpoint.trig_angle), -M_2PI / s->param->num_trig_tooth,
            M_2PI);
}

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

  s->mode = mode;

  /* 切换模式后重置PID和滤波器 */
  for (uint8_t i = 0; i < 2; i++) {
    PID_Reset(s->pid.fric + i);
    LowPassFilter2p_Reset(s->filter.in.fric + i, 0.0f);
    LowPassFilter2p_Reset(s->filter.out.fric + i, 0.0f);
  }
  PID_Reset(&(s->pid.trig));
  LowPassFilter2p_Reset(&(s->filter.in.trig), 0.0f);
  LowPassFilter2p_Reset(&(s->filter.out.trig), 0.0f);

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

  s->trig_timer_id = osTimerNew(TrigTimerCallback, osTimerPeriodic, s, NULL);

  for (uint8_t i = 0; i < 2; i++) {
    PID_Init(s->pid.fric + i, KPID_MODE_NO_D, target_freq,
             &(param->fric_pid_param));

    LowPassFilter2p_Init(s->filter.in.fric + i, target_freq,
                         param->low_pass_cutoff_freq.in.fric);

    LowPassFilter2p_Init(s->filter.out.fric + i, target_freq,
                         param->low_pass_cutoff_freq.out.fric);
  }

  PID_Init(&(s->pid.trig), KPID_MODE_CALC_D_FB, target_freq,
           &(param->trig_pid_param));

  LowPassFilter2p_Init(&(s->filter.in.trig), target_freq,
                       param->low_pass_cutoff_freq.in.trig);
  LowPassFilter2p_Init(&(s->filter.out.trig), target_freq,
                       param->low_pass_cutoff_freq.out.trig);
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
int8_t Shoot_Control(Shoot_t *s, CMD_ShootCmd_t *s_cmd, Referee_t *s_ref,
                     float dt_sec) {
  static uint32_t last_period_ms = 0;

  if (s == NULL) return -1;

  Shoot_SetMode(s, s_cmd->mode);

  if (s->mode == SHOOT_MODE_SAFE) {
    s_cmd->bullet_speed = 0.0f;
    s_cmd->shoot_freq_hz = 0.0f;
  }

  s->setpoint.fric_rpm[0] =
      s->param->bullet_speed_scaler * s_cmd->bullet_speed +
      s->param->bullet_speed_bias;
  s->setpoint.fric_rpm[1] = -s->setpoint.fric_rpm[0];

  /* 当裁判系统在线时启用热量控制与射速控制 */
  if (s_ref->ref_status == REF_STATUS_RUNNING && s->mode != SHOOT_MODE_SAFE) {
    float heat_percent; /* 当前热量与热量上限的比值 */
    float stable_freq_hz; /* 使机器人射击但热量不变化的射击频率 */

    float fric_radius_m = s->param->fric_radius_m; /* 摩擦轮半径 */
    float bullet_rpm_limit;                        /* 计算出的rpm上限 */
    uint8_t bullet_speed_limit; /* 裁判系统取得的射速上限 */

    if (s_ref->robot_status.robot_id == 1 ||
        s_ref->robot_status.robot_id == 101) {
      heat_percent = s_ref->power_heat.shoot_42_heat /
                     s_ref->robot_status.shoot_42_heat_limit;
      /* 每发射一颗42mm弹丸增加100热量 */
      stable_freq_hz = s_ref->robot_status.shoot_42_cooling_rate / 100.0f;
      bullet_speed_limit = s_ref->robot_status.shoot_42_speed_limit;
    } else {
      heat_percent = s_ref->power_heat.shoot_17_heat /
                     s_ref->robot_status.shoot_17_heat_limit;
      /* 每发射一颗17mm弹丸增加10热量 */
      stable_freq_hz = s_ref->robot_status.shoot_17_cooling_rate / 10.0f;
      bullet_speed_limit = s_ref->robot_status.shoot_17_speed_limit;
    }
    bullet_rpm_limit = CalculateRpm(bullet_speed_limit, fric_radius_m);
    s_cmd->shoot_freq_hz =
        HeatLimit_ShootFreq(heat_percent, stable_freq_hz, s_cmd->shoot_freq_hz);
    s->setpoint.fric_rpm[0] =
        AbsClip(s->setpoint.fric_rpm[0], bullet_rpm_limit);
    s->setpoint.fric_rpm[1] = -s->setpoint.fric_rpm[0];
  }

  if (s->mode != SHOOT_MODE_RELAX) {
    uint32_t period_ms = (uint32_t)(1000.0f / s_cmd->shoot_freq_hz);

    if (last_period_ms != period_ms || !osTimerIsRunning(s->trig_timer_id)) {
      last_period_ms = period_ms;
      if (osTimerIsRunning(s->trig_timer_id)) {
        osTimerStop(s->trig_timer_id);
      }
      TrigTimerCallback(s);
      osTimerStart(s->trig_timer_id, period_ms);
    }

  } else {
    if (osTimerIsRunning(s->trig_timer_id)) osTimerStop(s->trig_timer_id);
  }

  switch (s->mode) {
    case SHOOT_MODE_RELAX:
      s->out[0] = 0.0f;

      for (uint8_t i = 0; i < 2; i++) {
        s->out[i + 1] = 0.0f;
      }
      break;

    case SHOOT_MODE_SAFE:
    case SHOOT_MODE_STDBY:
    case SHOOT_MODE_FIRE:
      /* Filter feedback. */
      s->feedback.trig_angle =
          LowPassFilter2p_Apply(&(s->filter.in.trig), s->feedback.trig_angle);

      s->out[SHOOT_ACTR_TRIG_IDX] =
          PID_Calc(&(s->pid.trig), s->setpoint.trig_angle,
                   s->feedback.trig_angle, 0.0f, dt_sec);
      s->out[SHOOT_ACTR_TRIG_IDX] = LowPassFilter2p_Apply(
          &(s->filter.out.trig), s->out[SHOOT_ACTR_TRIG_IDX]);

      for (uint8_t i = 0; i < 2; i++) {
        /* Filter feedback. */
        s->feedback.fric_rpm[i] = LowPassFilter2p_Apply(
            &(s->filter.in.fric[i]), s->feedback.fric_rpm[i]);

        s->out[SHOOT_ACTR_FRIC1_IDX + i] =
            PID_Calc(&(s->pid.fric[i]), s->setpoint.fric_rpm[i],
                     s->feedback.fric_rpm[i], 0.0f, dt_sec);

        s->out[SHOOT_ACTR_FRIC1_IDX + i] = LowPassFilter2p_Apply(
            &(s->filter.out.fric[i]), s->out[SHOOT_ACTR_FRIC1_IDX + i]);
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
