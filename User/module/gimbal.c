/*
  云台模组
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
  g->mode = mode;

  return 0;
}

/* Exported functions --------------------------------------------------------*/
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param, float dt_sec) {
  if (g == NULL) return -1;

  g->param = param;
  g->dt_sec = dt_sec;

  g->mode = GIMBAL_MODE_RELAX;

  PID_Init(&(g->pid[GIMBAL_PID_YAW_IN_IDX]), PID_MODE_SET_D, g->dt_sec,
           &(g->param->pid[GIMBAL_PID_YAW_IN_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_YAW_OUT_IDX]), PID_MODE_NO_D, g->dt_sec,
           &(g->param->pid[GIMBAL_PID_YAW_OUT_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_PIT_IN_IDX]), PID_MODE_SET_D, g->dt_sec,
           &(g->param->pid[GIMBAL_PID_PIT_IN_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_PIT_OUT_IDX]), PID_MODE_NO_D, g->dt_sec,
           &(g->param->pid[GIMBAL_PID_PIT_OUT_IDX]));

  PID_Init(&(g->pid[GIMBAL_PID_REL_YAW_IDX]), PID_MODE_NO_D, g->dt_sec,
           &(g->param->pid[GIMBAL_PID_REL_YAW_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_REL_PIT_IDX]), PID_MODE_NO_D, g->dt_sec,
           &(g->param->pid[GIMBAL_PID_REL_PIT_IDX]));

  for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++)
    LowPassFilter2p_Init(&(g->filter[i]), 1.f / g->dt_sec,
                         g->param->low_pass_cutoff_freq);

  return 0;
}

int8_t Gimbal_CANtoFeedback(Gimbal_Feedback *gimbal_feedback, CAN_t *can) {
  if (gimbal_feedback == NULL) return -1;
  if (can == NULL) return -1;

  gimbal_feedback->eulr.encoder.yaw =
      can->gimbal_motor_feedback[CAN_MOTOR_GIMBAL_YAW].rotor_angle;
  gimbal_feedback->eulr.encoder.pit =
      can->gimbal_motor_feedback[CAN_MOTOR_GIMBAL_PIT].rotor_angle;

  return 0;
}

int8_t Gimbal_Control(Gimbal_t *g, Gimbal_Feedback *fb,
                      CMD_Gimbal_Ctrl_t *g_ctrl) {
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
          PID_Calc(&(g->pid[GIMBAL_PID_YAW_IN_IDX]), g->set_point.eulr.yaw,
                   fb->eulr.imu.yaw, fb->gyro.z, g->dt_sec);
      g->out[GIMBAL_ACTR_YAW_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_YAW_OUT_IDX]), motor_gyro_set,
                   fb->gyro.z, 0.f, g->dt_sec);

      motor_gyro_set =
          PID_Calc(&(g->pid[GIMBAL_PID_PIT_IN_IDX]), g->set_point.eulr.pit,
                   fb->eulr.imu.pit, fb->gyro.x, g->dt_sec);
      g->out[GIMBAL_ACTR_PIT_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_PIT_OUT_IDX]), motor_gyro_set,
                   fb->gyro.x, 0.f, g->dt_sec);
      break;

    case GIMBAL_MODE_FIX:
      g->set_point.eulr.yaw = g->param->encoder_center.yaw;
      g->set_point.eulr.pit = g->param->encoder_center.pit;
      /* NO break. */

    case GIMBAL_MODE_RELATIVE:
      g->out[GIMBAL_ACTR_YAW_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_REL_YAW_IDX]), g->set_point.eulr.yaw,
                   fb->eulr.encoder.yaw, fb->gyro.z, g->dt_sec);
      g->out[GIMBAL_ACTR_PIT_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_REL_PIT_IDX]), g->set_point.eulr.pit,
                   fb->eulr.encoder.pit, fb->gyro.x, g->dt_sec);
      break;
  }
  /* Filter output. */
  for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++)
    g->out[i] = LowPassFilter2p_Apply(&(g->filter[i]), g->out[i]);

  return 0;
}
