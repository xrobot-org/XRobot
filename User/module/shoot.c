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

  s->set_point.trig_angle += 2.f * M_PI / s->param->num_trig_tooth;
}

static int8_t Shoot_SetMode(Shoot_t *s, CMD_Shoot_Mode_t mode) {
  if (s == NULL) return -1;

  if (mode == s->mode) return SHOOT_OK;

  s->mode = mode;

  /* 切换模式后重置PID和滤波器 */
  for (uint8_t i = 0; i < 2; i++) {
    PID_ResetIntegral(&(s->pid.fric[i]));
    LowPassFilter2p_Reset(&(s->filter.fric[i]), 0.f);
  }
  PID_ResetIntegral(&(s->pid.trig));
  LowPassFilter2p_Reset(&(s->filter.trig), 0.f);

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
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *param, float dt_sec) {
  if (s == NULL) return -1;

  s->param = param;
  s->dt_sec = dt_sec;

  s->mode = SHOOT_MODE_RELAX;

  s->trig_timer_id = osTimerNew(TrigTimerCallback, osTimerPeriodic, s, NULL);

  for (uint8_t i = 0; i < 2; i++) {
    PID_Init(&(s->pid.fric[i]), PID_MODE_NO_D, s->dt_sec,
             &(param->fric_pid_param[i]));

    LowPassFilter2p_Init(&(s->filter.fric[i]), 1000.f / s->dt_sec,
                         param->low_pass_cutoff_freq.fric);
  }

  PID_Init(&(s->pid.trig), PID_MODE_NO_D, s->dt_sec, &(param->trig_pid_param));

  LowPassFilter2p_Init(&(s->filter.trig), 1.f / s->dt_sec,
                       param->low_pass_cutoff_freq.trig);
  return 0;
}

int8_t Shoot_UpdateFeedback(Shoot_t *s, CAN_t *can) {
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

int8_t Shoot_Control(Shoot_t *s, CMD_Shoot_Ctrl_t *s_ctrl) {
  if (s == NULL) return -1;

  Shoot_SetMode(s, s_ctrl->mode);

  if (s->mode == SHOOT_MODE_SAFE) {
    s_ctrl->bullet_speed = 0.f;
    s_ctrl->shoot_freq_hz = 0.f;
  }

  s->set_point.fric_rpm[0] =
      s->param->bullet_speed_scaler * s_ctrl->bullet_speed +
      s->param->bullet_speed_bias;
  s->set_point.fric_rpm[1] = -s->set_point.fric_rpm[0];

  uint32_t period_ms = 1000u / (uint32_t)s_ctrl->shoot_freq_hz;
  if (!osTimerIsRunning(s->trig_timer_id))
    osTimerStart(s->trig_timer_id, period_ms);

  switch (s->mode) {
    case SHOOT_MODE_RELAX:
      s->out[0] = 0.f;

      for (uint8_t i = 0; i < 2; i++) {
        s->out[i + 1] = 0.f;
      }
      break;

    case SHOOT_MODE_SAFE:
    case SHOOT_MODE_STDBY:
    case SHOOT_MODE_FIRE:
      s->out[0] = PID_Calc(&(s->pid.trig), s->set_point.trig_angle,
                           s->feedback.trig_angle, 0.f, s->dt_sec);
      s->out[0] = LowPassFilter2p_Apply(&(s->filter.trig), s->out[0]);

      for (uint8_t i = 0; i < 2; i++) {
        s->out[i + 1] =
            PID_Calc(&(s->pid.fric[i + 1]), s->set_point.fric_rpm[i + 1],
                     s->feedback.fric_rpm[i + 1], 0.f, s->dt_sec);
        s->out[i + 1] =
            LowPassFilter2p_Apply(&(s->filter.fric[i + 1]), s->out[i + 1]);
      }
      break;
  }
  return 0;
}
