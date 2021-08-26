/**
 * @file referee.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 裁判系统接收发送任务
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 * 接收来自裁判系统的数据
 * 解析后根据需求组合成新包发给各个模块
 * 无论裁判系统是否在线，都要按时发送给各个模块
 *
 */

/* Includes ----------------------------------------------------------------- */

#include "bsp_usb.h"
#include "dev_referee.h"
#include "thd.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

#ifdef MCU_DEBUG_BUILD
Referee_t ref;
Referee_ForCap_t for_cap;
Referee_ForAI_t for_ai;
Referee_ForChassis_t for_chassis;
Referee_ForLauncher_t for_launcher;
#else
static Referee_t ref;
static Referee_ForCap_t for_cap;
static Referee_ForAI_t for_ai;
static Referee_ForChassis_t for_chassis;
static Referee_ForLauncher_t for_launcher;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * @brief 裁判系统
 *
 * @param argument 未使用
 */
void Thread_Referee(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */
  const uint32_t delay_tick = pdMS_TO_TICKS(1000 / TASK_FREQ_REFEREE);

  /* 初始化裁判系统 */
  Referee_Init(&ref, &(runtime.cfg.pilot_cfg->screen));

  uint32_t tick = xTaskGetTickCount();
  while (1) {
    Referee_StartReceiving(&ref); /* 开始接收裁判系统数据 */

    if (Referee_WaitRecvCplt(100)) { /* 判断裁判系统数据是否接收完成 */
      Referee_HandleOffline(&ref); /* 长时间未接收到数据，裁判系统离线 */
    } else {
      Referee_Parse(&ref); /* 解析裁判系统数据 */
    }

    /* 定时接收发送数据 */
    if (xTaskGetTickCount() > tick) {
      tick += delay_tick;
      /* 打包裁判系统数据 */
      Referee_PackForCap(&for_cap, &ref);
      Referee_PackForAI(&for_ai, &ref);
      Referee_PackForLauncher(&for_launcher, &ref);
      Referee_PackForChassis(&for_chassis, &ref);

      /* 发送裁判系统数据到其他进程 */
      xQueueOverwrite(runtime.msgq.referee.cap, &for_cap);
      xQueueOverwrite(runtime.msgq.referee.ai, &for_ai);
      xQueueOverwrite(runtime.msgq.referee.chassis, &for_chassis);
      xQueueOverwrite(runtime.msgq.referee.launcher, &for_launcher);

      /* 获取其他进程数据用于绘制UI */
      xQueueReceive(runtime.msgq.ui.cap, &(ref.cap_ui), 0);
      xQueueReceive(runtime.msgq.ui.chassis, &(ref.chassis_ui), 0);
      xQueueReceive(runtime.msgq.ui.gimbal, &(ref.gimbal_ui), 0);
      xQueueReceive(runtime.msgq.ui.launcher, &(ref.launcher_ui), 0);
      xQueueReceive(runtime.msgq.ui.cmd, &(ref.cmd_ui), 0);
#if 0
      xQueueReceive(runtime.msgq.ui.ai, &(ref.ai_ui), 0);
#endif

      /* 刷新UI数据 */
      Referee_RefreshUI(&ref);

      if (Referee_WaitTransCplt(0)) {
        Referee_PackUiPacket(&ref);
      }
      Referee_StartTransmit(&ref);
    }
  }
}