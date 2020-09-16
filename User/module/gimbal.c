/*
        底盘模组

*/

/* Includes ------------------------------------------------------------------*/
#include "gimbal.h"

#include "bsp/mm.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
static int8_t Gimbal_SetMode(Gimbal_t *g, CMD_Gimbal_Mode_t mode) {
  if (g == NULL) return -1;

  if (mode == g->mode) return GIMBAL_OK;

  if (g->mode != GIMBAL_MODE_INIT) g->mode = mode;

  return 0;
}

static bool Gimbal_Steady(Gimbal_t *g) {
  static uint8_t steady;

  bool con1 = g->feedback.gyro.x < 0.1f;
  bool con2 = g->feedback.gyro.y < 0.1f;
  bool con3 = g->feedback.gyro.z < 0.1f;

  if (con1 && con2 && con3)
    steady++;
  else {
    return false;
  }
  if (steady > 20) {
    steady = 0;
    return true;
  }
  return false;
}

/* Exported functions --------------------------------------------------------*/
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param, float dt_sec) {
  if (g == NULL) return -1;

  g->mode = GIMBAL_MODE_INIT;
  g->dt_sec = dt_sec;

  PID_Init(&(g->pid[GIMBAL_PID_YAW_IN]), PID_MODE_SET_D, g->dt_sec,
           &(param->pid[GIMBAL_PID_YAW_IN]));
  PID_Init(&(g->pid[GIMBAL_PID_YAW_OUT]), PID_MODE_NO_D, g->dt_sec,
           &(param->pid[GIMBAL_PID_YAW_OUT]));
  PID_Init(&(g->pid[GIMBAL_PID_PIT_IN]), PID_MODE_SET_D, g->dt_sec,
           &(param->pid[GIMBAL_PID_PIT_IN]));
  PID_Init(&(g->pid[GIMBAL_PID_PIT_OUT]), PID_MODE_NO_D, g->dt_sec,
           &(param->pid[GIMBAL_PID_PIT_OUT]));
  
  PID_Init(&(g->pid[GIMBAL_PID_REL_YAW]), PID_MODE_SET_D, g->dt_sec,
           &(param->pid[GIMBAL_PID_REL_YAW]));
  PID_Init(&(g->pid[GIMBAL_PID_REL_PIT]), PID_MODE_NO_D, g->dt_sec,
           &(param->pid[GIMBAL_PID_REL_PIT]));

  for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++)
    LowPassFilter2p_Init(&(g->filter[i]), 1.f / g->dt_sec,
                         param->low_pass_cutoff);

  return 0;
}

int8_t Gimbal_UpdateFeedback(Gimbal_t *g, CAN_t *can) {
  if (g == NULL) return -1;

  if (can == NULL) return -1;

  g->feedback.eulr.encoder.yaw =
      can->gimbal_motor_feedback[CAN_MOTOR_GIMBAL_YAW].rotor_angle;
  g->feedback.eulr.encoder.pit =
      can->gimbal_motor_feedback[CAN_MOTOR_GIMBAL_PIT].rotor_angle;

  return 0;
}

int8_t Gimbal_Control(Gimbal_t *g, CMD_Gimbal_Ctrl_t *g_ctrl) {
  if (g == NULL) return -1;

  if (g_ctrl == NULL) return -1;

  Gimbal_SetMode(g, g_ctrl->mode);

  g->set_point.eulr.yaw += g_ctrl->delta_eulr.yaw;
  g->set_point.eulr.pit += g_ctrl->delta_eulr.pit;

  float motor_gyro_set;
  switch (g->mode) {
    case GIMBAL_MODE_RELAX:
      for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++) g->out[i] = 0.f;
      break;

    case GIMBAL_MODE_ABSOLUTE:
      motor_gyro_set =
          PID_Calc(&(g->pid[GIMBAL_PID_YAW_IN]), g->set_point.eulr.yaw,
                   g->feedback.eulr.imu.yaw, g->feedback.gyro.z, g->dt_sec);
      g->out[GIMBAL_ACTR_YAW] =
          PID_Calc(&(g->pid[GIMBAL_PID_YAW_OUT]), motor_gyro_set,
                   g->feedback.gyro.z, 0.f, g->dt_sec);

      motor_gyro_set =
          PID_Calc(&(g->pid[GIMBAL_PID_PIT_IN]), g->set_point.eulr.pit,
                   g->feedback.eulr.imu.pit, g->feedback.gyro.x, g->dt_sec);
      g->out[GIMBAL_ACTR_PIT] =
          PID_Calc(&(g->pid[GIMBAL_PID_PIT_OUT]), motor_gyro_set,
                   g->feedback.gyro.x, 0.f, g->dt_sec);
      break;

    case GIMBAL_MODE_INIT:
      if (Gimbal_Steady(g)) g->mode = GIMBAL_MODE_RELAX;
      break;

    case GIMBAL_MODE_FIX:
      g->set_point.eulr.pit = 0.f;
      g->set_point.eulr.pit = 0.f;
      /* NO break. */

    case GIMBAL_MODE_RELATIVE:
      g->out[GIMBAL_ACTR_YAW] =
          PID_Calc(&(g->pid[GIMBAL_PID_REL_YAW]), g->set_point.eulr.yaw,
                   g->feedback.eulr.encoder.yaw, g->feedback.gyro.z, g->dt_sec);
      g->out[GIMBAL_ACTR_PIT] =
          PID_Calc(&(g->pid[GIMBAL_PID_REL_PIT]), g->set_point.eulr.pit,
                   g->feedback.eulr.encoder.pit, g->feedback.gyro.x, g->dt_sec);
      break;
  }
  /* Filter output. */
  for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++)
    g->out[i] = LowPassFilter2p_Apply(&(g->filter[i]), g->out[i]);

  return 0;
}
