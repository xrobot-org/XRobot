/**
 * @file dart.c
 * @author Qu Shen
 * @brief 飞镖控制模组
 * @version 0.1
 * @date 2021-07-11
 *
 * @copyright Copyright (c) 2021
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "dart.h"

#include "bsp/mm.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

/**
 * @brief 设置飞镖模式
 *
 * @param c 包含飞镖数据的结构体
 * @param mode 要设置的模式
 *
 * @return 函数运行结果
 */
static void Dart_ReadyControl(Dart_t *d) {
  ASSERT(d);
  /* 读取气压计，计算高度 */
}

static void Dart_AscendControl(Dart_t *d) {
  ASSERT(d);
  /* 根据目标能量控制飞镖推进力 */
  /* 不进行姿态控制，减少能量损耗 */
}

static void Dart_DescendControl(Dart_t *d) {
  ASSERT(d);
  /* 读取摄像头数据 */
  /* 计算目标落点 */
  /* 计算目标能量 */
  /* 估计现有能量 */
  /*  */
}

static void Dart_LandControl(Dart_t *d) {
  ASSERT(d);
  if (mode == d->mode) return GIMBAL_OK;
}

static void Dart_UpdateStage(Dart_t *d) {
  ASSERT(d);
  if (mode == d->mode) return GIMBAL_OK;
}

/* Exported functions ------------------------------------------------------- */

/**
 * @brief 初始化飞镖
 *
 * @param d 包含飞镖数据的结构体
 * @param param 包含飞镖参数的结构体指针
 * @param target_freq 任务预期的运行频率
 */
void Dart_Init(Dart_t *d, const Dart_Params_t *param, float limit_max,
               float target_freq) {
  ASSERT(d);
  ASSERT(param);

  d->param = param;            /* 初始化参数 */
  d->mode = GIMBAL_MODE_RELAX; /* 设置默认模式 */

  /* 设置软件限位 */
  if (d->param->reverse.pit) CircleReverse(&limit_max);
  d->limit.min = d->limit.max = limit_max;
  CircleAdd(&(d->limit.min), -d->param->pitch_travel_rad, M_2PI);

  /* 初始化飞镖电机控制PID和LPF */
  PID_Init(&(d->pid[GIMBAL_CTRL_YAW_ANGLE_IDX]), KPID_MODE_NO_D, target_freq,
           &(d->param->pid[GIMBAL_CTRL_YAW_ANGLE_IDX]));
  PID_Init(&(d->pid[GIMBAL_CTRL_YAW_OMEGA_IDX]), KPID_MODE_CALC_D, target_freq,
           &(d->param->pid[GIMBAL_CTRL_YAW_OMEGA_IDX]));

  PID_Init(&(d->pid[GIMBAL_CTRL_PIT_ANGLE_IDX]), KPID_MODE_NO_D, target_freq,
           &(d->param->pid[GIMBAL_CTRL_PIT_ANGLE_IDX]));
  PID_Init(&(d->pid[GIMBAL_CTRL_PIT_OMEGA_IDX]), KPID_MODE_CALC_D, target_freq,
           &(d->param->pid[GIMBAL_CTRL_PIT_OMEGA_IDX]));

  for (size_t i = 0; i < GIMBAL_ACTR_NUM; i++) {
    LowPassFilter2p_Init(d->filter_out + i, target_freq,
                         d->param->low_pass_cutoff_freq.out);
  }
  return 0;
}

/**
 * @brief 通过CAN设备更新飞镖反馈信息
 *
 * @param d 飞镖
 * @param can CAN设备
 */
void Dart_UpdateFeedback(Dart_t *d, const CAN_t *can) {
  ASSERT(d);
  ASSERT(can);

  d->feedback.eulr.encoder.yaw = can->motor.dart.named.yaw.rotor_abs_angle;
  d->feedback.eulr.encoder.pit = can->motor.dart.named.pit.rotor_abs_angle;

  if (d->param->reverse.yaw) CircleReverse(&(d->feedback.eulr.encoder.yaw));
  if (d->param->reverse.pit) CircleReverse(&(d->feedback.eulr.encoder.pit));

  return 0;
}

/**
 * @brief 运行飞镖控制逻辑
 *
 * @param d 包含飞镖数据的结构体
 * @param fb 飞镖反馈信息
 * @param g_cmd 飞镖控制指令
 * @param dt_sec 两次调用的时间间隔
 */
void Dart_Control(Dart_t *d, CMD_GimbalCmd_t *g_cmd, uint32_t now) {
  ASSERT(d);
  ASSERT(g_cmd);

  d->dt = (float)(now - d->lask_wakeup) / 1000.0f;
  d->lask_wakeup = now;

  Dart_UpdateStage(d);

  /* 控制相关逻辑 */
  switch (d->stage) {
    case DART_STAGE_READY:
      Dart_ReadyControl(d);
      break;

    case DART_STAGE_ASCEND:
      Dart_AscendControl(d);
      break;

    case DART_STAGE_DESCEND:
      Dart_DescendControl(d);
      break;

    case DART_STAGE_LAND:
      Dart_LandControl(d);
      break;
  }
}
