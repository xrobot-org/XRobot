/*
 * 云台模组
 */

/* Includes ----------------------------------------------------------------- */
#include "gimbal.h"

#include "bsp/mm.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

/**
 * \brief 设置云台模式
 *
 * \param c 包含云台数据的结构体
 * \param mode 要设置的模式
 *
 * \return 函数运行结果
 */
static int8_t Gimbal_SetMode(Gimbal_t *g, CMD_GimbalMode_t mode) {
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

  AHRS_ResetEulr(&(g->setpoint.eulr)); /* 切换模式后重置设定值 */

  return 0;
}

/* Exported functions ------------------------------------------------------- */

/**
 * \brief 初始化云台
 *
 * \param g 包含云台数据的结构体
 * \param param 包含云台参数的结构体指针
 * \param target_freq 任务预期的运行频率
 *
 * \return 函数运行结果
 */
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param, float limit_max,
                   float target_freq) {
  if (g == NULL) return -1;

  g->param = param;                /* 初始化参数 */
  g->mode = GIMBAL_MODE_RELAX;     /* 设置默认模式 */
  g->gimbal_limit.max = limit_max; /* 设置软件限位 */
  g->gimbal_limit.min = limit_max - g->param->gimbal_limit_radian_pitch;

  /* 初始化云台电机控制PID和LPF */
  PID_Init(&(g->pid[GIMBAL_PID_YAW_ANGLE_IDX]), KPID_MODE_NO_D, target_freq,
           &(g->param->pid[GIMBAL_PID_YAW_ANGLE_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_YAW_OMEGA_IDX]), KPID_MODE_CALC_D_FB,
           target_freq, &(g->param->pid[GIMBAL_PID_YAW_OMEGA_IDX]));

  PID_Init(&(g->pid[GIMBAL_PID_PIT_ANGLE_IDX]), KPID_MODE_NO_D, target_freq,
           &(g->param->pid[GIMBAL_PID_PIT_ANGLE_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_PIT_OMEGA_IDX]), KPID_MODE_CALC_D_FB,
           target_freq, &(g->param->pid[GIMBAL_PID_PIT_OMEGA_IDX]));

  PID_Init(&(g->pid[GIMBAL_PID_REL_YAW_IDX]), KPID_MODE_NO_D, target_freq,
           &(g->param->pid[GIMBAL_PID_REL_YAW_IDX]));
  PID_Init(&(g->pid[GIMBAL_PID_REL_PIT_IDX]), KPID_MODE_NO_D, target_freq,
           &(g->param->pid[GIMBAL_PID_REL_PIT_IDX]));

  for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++) {
    LowPassFilter2p_Init(g->filter_out + i, target_freq,
                         g->param->low_pass_cutoff_freq.out);
  }
  return 0;
}

/**
 * \brief 通过CAN设备更新云台反馈信息
 *
 * \param gimbal 云台
 * \param can CAN设备
 *
 * \return 函数运行结果
 */
int8_t Gimbal_UpdateFeedback(Gimbal_t *gimbal, const CAN_t *can) {
  if (gimbal == NULL) return -1;
  if (can == NULL) return -1;

  gimbal->feedback.eulr.encoder.yaw = can->motor.gimbal.named.yaw.rotor_angle;
  gimbal->feedback.eulr.encoder.pit = can->motor.gimbal.named.pit.rotor_angle;

  if (gimbal->param->reverse.yaw)
    gimbal->feedback.eulr.encoder.yaw =
        -gimbal->feedback.eulr.encoder.yaw + M_2PI;
  if (gimbal->param->reverse.pit)
    gimbal->feedback.eulr.encoder.pit =
        -gimbal->feedback.eulr.encoder.pit + M_2PI;

  return 0;
}

/**
 * \brief 运行云台控制逻辑
 *
 * \param g 包含云台数据的结构体
 * \param fb 云台反馈信息
 * \param g_cmd 云台控制指令
 * \param dt_sec 两次调用的时间间隔
 *
 * \return 函数运行结果
 */
int8_t Gimbal_Control(Gimbal_t *g, CMD_GimbalCmd_t *g_cmd, float dt_sec) {
  if (g == NULL) return -1;
  if (g_cmd == NULL) return -1;

  Gimbal_SetMode(g, g_cmd->mode);

  /* yaw坐标正方向与遥控器操作逻辑相反 */
  g_cmd->delta_eulr.pit = (g_cmd->delta_eulr.pit);
  g_cmd->delta_eulr.yaw = -(g_cmd->delta_eulr.yaw);

  /* 设置初始yaw目标值 */
  if (g->setpoint.eulr.yaw == 0.0f) {
    g->setpoint.eulr.yaw = g->feedback.eulr.imu.yaw;
  }

  /* 处理控制命令，限制setpoint范围 */
  CircleAdd(&(g->setpoint.eulr.yaw), g_cmd->delta_eulr.yaw, M_2PI);

  /* pitch轴软件限位 */
  float delta_max =
      CircleError(g->gimbal_limit.max,
                  (g->feedback.eulr.encoder.pit + g->setpoint.eulr.pit -
                   g->feedback.eulr.imu.pit),
                  M_2PI);
  float delta_min =
      CircleError(g->gimbal_limit.min,
                  (g->feedback.eulr.encoder.pit + g->setpoint.eulr.pit -
                   g->feedback.eulr.imu.pit),
                  M_2PI);
  if (g_cmd->delta_eulr.pit > delta_max) g_cmd->delta_eulr.pit = delta_max;
  if (g_cmd->delta_eulr.pit < delta_min) g_cmd->delta_eulr.pit = delta_min;
  g->setpoint.eulr.pit += g_cmd->delta_eulr.pit;

  AHRS_ResetEulr(&(g_cmd->delta_eulr));

  /* 控制相关逻辑 */
  float yaw_omega_set_point, pit_omega_set_point;
  switch (g->mode) {
    case GIMBAL_MODE_RELAX:
      for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++) g->out[i] = 0.0f;
      break;

    case GIMBAL_MODE_ABSOLUTE:
      yaw_omega_set_point =
          PID_Calc(&(g->pid[GIMBAL_PID_YAW_ANGLE_IDX]), g->setpoint.eulr.yaw,
                   g->feedback.eulr.imu.yaw, 0.0f, dt_sec);
      g->out[GIMBAL_ACTR_YAW_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_YAW_OMEGA_IDX]), yaw_omega_set_point,
                   g->feedback.gyro.z, 0.f, dt_sec);

      pit_omega_set_point =
          PID_Calc(&(g->pid[GIMBAL_PID_PIT_ANGLE_IDX]), g->setpoint.eulr.pit,
                   g->feedback.eulr.imu.pit, 0.0f, dt_sec);
      g->out[GIMBAL_ACTR_PIT_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_PIT_OMEGA_IDX]), pit_omega_set_point,
                   g->feedback.gyro.x, 0.f, dt_sec);
      break;

    case GIMBAL_MODE_FIX:
      g->setpoint.eulr.yaw = g->param->encoder_center.yaw;
      g->setpoint.eulr.pit = g->param->encoder_center.pit;
      /* 这里不要加break */

    case GIMBAL_MODE_RELATIVE:
      g->out[GIMBAL_ACTR_YAW_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_REL_YAW_IDX]), g->setpoint.eulr.yaw,
                   g->feedback.eulr.encoder.yaw, g->feedback.gyro.z, dt_sec);
      g->out[GIMBAL_ACTR_PIT_IDX] =
          PID_Calc(&(g->pid[GIMBAL_PID_REL_PIT_IDX]), g->setpoint.eulr.pit,
                   g->feedback.eulr.encoder.pit, g->feedback.gyro.x, dt_sec);
      break;
  }

  /* 输出滤波 */
  for (uint8_t i = 0; i < GIMBAL_ACTR_NUM; i++)
    g->out[i] = LowPassFilter2p_Apply(g->filter_out + i, g->out[i]);

  return 0;
}

/**
 * \brief 复制云台输出值
 *
 * \param s 包含云台数据的结构体
 * \param out CAN设备云台输出结构体
 */
void Gimbal_DumpOutput(Gimbal_t *g, CAN_GimbalOutput_t *out) {
  out->named.yaw = g->out[GIMBAL_ACTR_YAW_IDX];
  out->named.pit = g->out[GIMBAL_ACTR_PIT_IDX];
}
