/*
  云台模组
*/

/* Includes ----------------------------------------------------------------- */
#include "gimbal.h"

#include "bsp/mm.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */
static int8_t Gimbal_SetMode(Gimbal_t *g, CMD_Gimbal_Mode_t mode) {
  if (g == NULL) return -1;
  if (mode == g->mode) return GIMBAL_OK;
  g->mode = mode;

  /* 切换模式后重置PID和滤波器 */
  for (uint8_t i = 0; i < GIMBAL_PID_NUM; i++) {
    PID_Reset(g->pid + i);
  }
  for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++) {
    LowPassFilter2p_Reset(g->filter_out + i, 0.0f);
  }
  for (uint8_t i = 0; i < 2; i++) {
    LowPassFilter2p_Reset(g->filter_gyro + i, 0.0f);
  }

  AHRS_ResetEulr(&(g->set_point.eulr));

  return 0;
}

/* Exported functions ------------------------------------------------------- */
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param, float target_freq) {
  if (g == NULL) return -1;

  g->param = param;

  g->mode = GIMBAL_MODE_RELAX;

  PID_Init(&(g->pid[GIMBAL_PID_YAW_ANGLE_IDX]), PID_MODE_NO_D, 1.0f / target_freq,
           &(g->param->pid[GIMBAL_PID_YAW_ANGLE_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_YAW_OMEGA_IDX]), PID_MODE_SET_D, 1.0f / target_freq,
           &(g->param->pid[GIMBAL_PID_YAW_OMEGA_IDX]));

  PID_Init(&(g->pid[GIMBAL_PID_PIT_ANGLE_IDX]), PID_MODE_NO_D, 1.0f / target_freq,
           &(g->param->pid[GIMBAL_PID_PIT_ANGLE_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_PIT_OMEGA_IDX]), PID_MODE_SET_D, 1.0f / target_freq,
           &(g->param->pid[GIMBAL_PID_PIT_OMEGA_IDX]));

  PID_Init(&(g->pid[GIMBAL_PID_REL_YAW_IDX]), PID_MODE_NO_D, 1.0f / target_freq,
           &(g->param->pid[GIMBAL_PID_REL_YAW_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_REL_PIT_IDX]), PID_MODE_NO_D, 1.0f / target_freq,
           &(g->param->pid[GIMBAL_PID_REL_PIT_IDX]));

  for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++) {
    LowPassFilter2p_Init(g->filter_out + i, target_freq,
                         g->param->out_low_pass_cutoff_freq);
  }

  for (uint8_t i = 0; i < 2; i++) {
    LowPassFilter2p_Init(g->filter_gyro + i, target_freq,
                         g->param->gyro_low_pass_cutoff_freq);
  }

  g->set_point.eulr.yaw = 70.0f;

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
                      CMD_Gimbal_Ctrl_t *g_ctrl, float dt_sec) {
  if (g == NULL) return -1;
  if (g_ctrl == NULL) return -1;

  Gimbal_SetMode(g, g_ctrl->mode);

  if (g->set_point.eulr.yaw == 0.0f) {
    g->set_point.eulr.yaw = fb->eulr.imu.yaw;
  }

  g->set_point.eulr.yaw += g_ctrl->delta_eulr.yaw;
  g->set_point.eulr.pit += g_ctrl->delta_eulr.pit;

  if (g->set_point.eulr.yaw < -180.0f) {
    g->set_point.eulr.yaw += 360.0f;
  }
  if (g->set_point.eulr.yaw > 180.0f) {
    g->set_point.eulr.yaw -= 360.0f;
  }
  g->set_point.eulr.pit = AbsClip(g->set_point.eulr.pit, 90.0f);

  float filted_gyro_x, filted_gyro_z;
  switch (g->mode) {
    case GIMBAL_MODE_RELAX:
      for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++) g->out[i] = 0.0f;
      break;

    case GIMBAL_MODE_ABSOLUTE:
      /* Filter gyro. */
      filted_gyro_x = LowPassFilter2p_Apply(&(g->filter_gyro[0]), fb->gyro.x);
      filted_gyro_z = LowPassFilter2p_Apply(&(g->filter_gyro[1]), fb->gyro.z);

      const float yaw_omega_set_point =
          PID_Calc(&(g->pid[GIMBAL_PID_YAW_ANGLE_IDX]), g->set_point.eulr.yaw,
                   fb->eulr.imu.yaw, 0.0f, dt_sec);
      g->out[GIMBAL_ACTR_YAW_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_YAW_OMEGA_IDX]), yaw_omega_set_point,
                   fb->gyro.z, filted_gyro_z, dt_sec);

      const float pit_omega_set_point =
          PID_Calc(&(g->pid[GIMBAL_PID_PIT_ANGLE_IDX]), g->set_point.eulr.pit,
                   fb->eulr.imu.pit, 0.0f, dt_sec);
      g->out[GIMBAL_ACTR_PIT_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_PIT_OMEGA_IDX]), pit_omega_set_point,
                   fb->gyro.x, filted_gyro_x, dt_sec);
      break;

    case GIMBAL_MODE_FIX:
      g->set_point.eulr.yaw = g->param->encoder_center.yaw;
      g->set_point.eulr.pit = g->param->encoder_center.pit;
      /* NO break. */

    case GIMBAL_MODE_RELATIVE:
      g->out[GIMBAL_ACTR_YAW_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_REL_YAW_IDX]), g->set_point.eulr.yaw,
                   fb->eulr.encoder.yaw, fb->gyro.z, dt_sec);
      g->out[GIMBAL_ACTR_PIT_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_REL_PIT_IDX]), g->set_point.eulr.pit,
                   fb->eulr.encoder.pit, fb->gyro.x, dt_sec);
      break;
  }
  /* Filter output. */
  for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++)
    g->out[i] = LowPassFilter2p_Apply(g->filter_out + i, g->out[i]);

  return 0;
}
