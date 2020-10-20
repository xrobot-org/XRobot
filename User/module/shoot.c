/*
  射击模组
*/

/* Includes ----------------------------------------------------------------- */
#include "shoot.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */
static void TrigTimerCallback(void *arg) {
  Shoot_t *s = (Shoot_t *)arg;
  CircleAdd(&(s->setpoint.trig_angle), M_2PI / s->param->num_trig_tooth, M_2PI);
}

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

  // TODO: Check mode switchable.
  switch (mode) {
    case SHOOT_MODE_RELAX:
      break;

    case SHOOT_MODE_SAFE:
      break;

    case SHOOT_MODE_STDBY:
      break;

    case SHOOT_MODE_FIRE:
      break;
  }
  return 0;
}

/* Exported functions ------------------------------------------------------- */
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *param, float target_freq) {
  if (s == NULL) return -1;

  s->param = param;

  s->mode = SHOOT_MODE_RELAX;

  s->trig_timer_id = osTimerNew(TrigTimerCallback, osTimerPeriodic, s, NULL);

  for (uint8_t i = 0; i < 2; i++) {
    PID_Init(s->pid.fric + i, KPID_MODE_NO_D, target_freq,
             &(param->fric_pid_param));

    LowPassFilter2p_Init(s->filter.in.fric + i, target_freq,
                         param->low_pass_cutoff_freq.in.fric);

    LowPassFilter2p_Init(s->filter.out.fric + i, target_freq,
                         param->low_pass_cutoff_freq.out.fric);
  }

  PID_Init(&(s->pid.trig), KPID_MODE_NO_D, target_freq,
           &(param->trig_pid_param));

  LowPassFilter2p_Init(&(s->filter.in.trig), target_freq,
                       param->low_pass_cutoff_freq.in.trig);
  LowPassFilter2p_Init(&(s->filter.out.trig), target_freq,
                       param->low_pass_cutoff_freq.out.trig);
  return 0;
}

int8_t Shoot_UpdateFeedback(Shoot_t *s, const CAN_t *can) {
  if (s == NULL) return -1;

  if (can == NULL) return -1;

  for (uint8_t i = 0; i < 2; i++) {
    s->feedback.fric_rpm[i] =
        can->shoot_motor_feedback[CAN_MOTOR_SHOOT_FRIC1 + i].rotor_speed;
  }

  s->feedback.trig_angle =
      can->shoot_motor_feedback[CAN_MOTOR_SHOOT_TRIG].rotor_angle;

  return 0;
}

int8_t Shoot_Control(Shoot_t *s, CMD_ShootCmd_t *s_cmd, float dt_sec) {
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

  const uint32_t period_ms = 1000u / (uint32_t)s_cmd->shoot_freq_hz;
  if (period_ms > 0) {
    if (!osTimerIsRunning(s->trig_timer_id))
      osTimerStart(s->trig_timer_id, period_ms);
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
