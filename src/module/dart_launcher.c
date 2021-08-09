/**
 * @file launcher.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 小弹丸飞镖发射器模块
 * @version 1.0.0
 * @date 2021-05-04
 *
 * @copyright Copyright (c) 2021
 *
 */

/* Includes ----------------------------------------------------------------- */
#include "dart_launcher.h"

#include "bsp/pwm.h"
#include "component/game.h"
#include "component/limiter.h"
#include "component/utils.h"
/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */

/**
 * @brief 设置飞镖发射器模式
 *
 * @param dl 包含飞镖发射器数据的结构体
 * @param mode 要设置的模式
 */
static void DartLauncher_SetMode(DartLauncher_t *dl,
                                 Game_DartLauncherMode_t mode) {
  ASSERT(dl);

  if (mode == dl->mode) return;

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < DART_LAUNCHER_FRIC_NUM; i++) {
    PID_Reset(dl->pid.fric + i);
    LowPassFilter2p_Reset(dl->filter.in.fric + i, 0.0f);
    LowPassFilter2p_Reset(dl->filter.out.fric + i, 0.0f);
  }
  for (size_t j = 0; j < DART_LAUNCHER_FRIC_NUM; j++) {
    PID_Reset(dl->pid.fly + j);
    LowPassFilter2p_Reset(dl->filter.in.fly + j, 0.0f);
    LowPassFilter2p_Reset(dl->filter.out.fly + j, 0.0f);
  }
  PID_Reset(&(dl->pid.feed));
  LowPassFilter2p_Reset(&(dl->filter.in.feed), 0.0f);
  LowPassFilter2p_Reset(&(dl->filter.out.feed), 0.0f);

  dl->mode = mode;
}

/* Exported functions -------------------------------------------------------
 */

/**
 * @brief 初始化飞镖发射器
 *
 * @param dl 包含飞镖发射器数据的结构体
 * @param param 包含飞镖发射器参数的结构体指针
 * @param target_freq 任务预期的运行频率
 */
void DartLauncher_Init(DartLauncher_t *dl, const DartLauncher_Params_t *param,
                       float target_freq) {
  ASSERT(dl);
  ASSERT(param);

  dl->param = param;              /* 初始化参数 */
  dl->mode = LAUNCHER_MODE_RELAX; /* 设置默认模式 */

  for (size_t i = 0; i < DART_LAUNCHER_FRIC_NUM; i++) {
    /* PI控制器初始化PID */
    PID_Init(dl->pid.fric + i, KPID_MODE_NO_D, target_freq,
             &(param->fric_pid_param));

    LowPassFilter2p_Init(dl->filter.in.fric + i, target_freq,
                         param->low_pass_cutoff_freq.in.fric);

    LowPassFilter2p_Init(dl->filter.out.fric + i, target_freq,
                         param->low_pass_cutoff_freq.out.fric);
  }
  for (size_t i = 0; i < DART_LAUNCHER_FLY_NUM; i++) {
    PID_Init(dl->pid.fly + i, KPID_MODE_CALC_D, target_freq,
             &(param->trig_pid_param));

    LowPassFilter2p_Init(dl->filter.in.fly + i, target_freq,
                         param->low_pass_cutoff_freq.in.fly);
    LowPassFilter2p_Init(dl->filter.out.fly + i, target_freq,
                         param->low_pass_cutoff_freq.out.fly);
  }
  PID_Init(&(dl->pid.feed), KPID_MODE_CALC_D, target_freq,
           &(param->trig_pid_param));

  LowPassFilter2p_Init(&(dl->filter.in.feed), target_freq,
                       param->low_pass_cutoff_freq.in.feed);
  LowPassFilter2p_Init(&(dl->filter.out.feed), target_freq,
                       param->low_pass_cutoff_freq.out.feed);
}

/**
 * @brief 更新飞镖发射器的反馈信息
 *
 * @param dl 包含飞镖发射器数据的结构体
 * @param can CAN设备结构体
 */
void DartLauncher_UpdateFeedback(DartLauncher_t *dl, const CAN_t *can) {
  ASSERT(dl);
  ASSERT(can);

  /* Not implemented */
  ASSERT(0);
}

/**
 * @brief 运行飞镖发射器控制逻辑
 *
 * @param dl 包含飞镖发射器数据的结构体
 * @param l_cmd 飞镖发射器控制指令
 * @param l_ref 飞镖发射器使用的裁判系统数据
 * @param now 现在时刻
 */
void DartLauncher_Control(DartLauncher_t *dl, Referee_ForDartLauncher_t *dl_ref,
                          uint32_t now) {
  ASSERT(dl);
  ASSERT(dl_ref);

  dl->dt = (float)(now - dl->lask_wakeup) / 1000.0f;
  dl->lask_wakeup = now;

  /* Not implemented */
  ASSERT(0);

  switch (dl->mode) {
    case LAUNCHER_MODE_RELAX:
      for (size_t i = 0; i < DART_LAUNCHER_FRIC_NUM; i++) {
        dl->out.fric[i] = 0;
      }
      for (size_t i = 0; i < DART_LAUNCHER_FLY_NUM; i++) {
        dl->out.fly[i] = 0;
      }
      dl->out.feed = 0;
      break;

    case LAUNCHER_MODE_SAFE:
      for (size_t i = 0; i < DART_LAUNCHER_FRIC_NUM; i++) {
        dl->setpoint.fric_rpm[i] = 0;
      }
      for (size_t i = 0; i < DART_LAUNCHER_FLY_NUM; i++) {
        dl->setpoint.fly_rpm[i] = 0;
      }
    case LAUNCHER_MODE_LOADED:
      for (size_t i = 0; i < DART_LAUNCHER_FRIC_NUM; i++) {
        /* 控制摩擦轮 */
        dl->feedback.fric_rpm[i] = LowPassFilter2p_Apply(
            dl->filter.in.fric + i, dl->feedback.fric_rpm[i]);

        dl->out.fric[i] = PID_Calc(dl->pid.fric + i, dl->setpoint.fric_rpm[i],
                                   dl->feedback.fric_rpm[i], 0.0f, dl->dt);

        dl->out.fric[i] =
            LowPassFilter2p_Apply(dl->filter.out.fric + i, dl->out.fric[i]);
      }

      for (size_t i = 0; i < DART_LAUNCHER_FLY_NUM; i++) {
        dl->feedback.fly_rpm[i] = LowPassFilter2p_Apply(
            dl->filter.in.fly + i, dl->feedback.fly_rpm[i]);

        dl->out.fly[i] = PID_Calc(dl->pid.fly + i, dl->setpoint.fly_rpm[i],
                                  dl->feedback.fly_rpm[i], 0.0f, dl->dt);

        dl->out.fly[i] =
            LowPassFilter2p_Apply(dl->filter.out.fly + i, dl->out.fly[i]);
      }
      dl->feedback.feed_pos =
          LowPassFilter2p_Apply(&(dl->filter.in.feed), dl->feedback.feed_pos);

      dl->out.feed = PID_Calc(&(dl->pid.feed), dl->setpoint.feed_pos,
                              dl->feedback.feed_pos, 0.0f, dl->dt);

      dl->out.feed =
          LowPassFilter2p_Apply(&(dl->filter.out.feed), dl->out.feed);
      break;
  }
}

/**
 * @brief 复制飞镖发射器输出值
 *
 * @param dl 包含飞镖发射器数据的结构体
 * @param out CAN设备飞镖发射器输出结构体
 */
void DartLauncher_PackOutput(DartLauncher_t *dl,
                             CAN_DartLauncherOutput_t *out) {
  ASSERT(dl);
  ASSERT(out);

  /* Not implemented */
  ASSERT(0);
}

/**
 * @brief 清空输出值
 *
 * @param output 要清空的结构体
 */
void DartLauncher_ResetOutput(CAN_DartLauncherOutput_t *output) {
  ASSERT(output);
  memset(output, 0, sizeof(*output));
}
