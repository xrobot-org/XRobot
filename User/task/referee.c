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
 *
 */

/* Includes ----------------------------------------------------------------- */

#include "device/referee.h"

#include "bsp/usb.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */

#ifdef DEBUG
Referee_t ref;
Referee_UI_t ui;
CMD_UI_t ref_cmd;
Referee_ForCap_t for_cap;
Referee_ForAI_t for_ai;
Referee_ForChassis_t for_chassis;
Referee_ForLauncher_t for_launcher;
#else
static Referee_t ref;
static Referee_UI_t ui;
static CMD_UI_t ref_cmd;
static Referee_ForCap_t for_cap;
static Referee_ForAI_t for_ai;
static Referee_ForChassis_t for_chassis;
static Referee_ForLauncher_t for_launcher;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief 裁判系统
 *
 * \param argument 未使用
 */
void Task_Referee(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_REFEREE;

  /* 初始化裁判系统 */
  Referee_Init(&ref, &ui, &(task_runtime.cfg.pilot_cfg->param.screen));

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.referee =
        osThreadGetStackSpace(osThreadGetId());
#endif
    /* 开始接收裁判系统数据 */
    Referee_StartReceiving(&ref);
    /* 判断裁判系统数据是否接收完成 */
    if (osThreadFlagsWait(SIGNAL_REFEREE_RAW_REDY, osFlagsWaitAll, 100) !=
        SIGNAL_REFEREE_RAW_REDY) {
      /* 长时间未接收到数据，裁判系统离线 */
      Referee_HandleOffline(&ref);
    } else {
      /* 解析裁判系统数据 */
      Referee_Parse(&ref);

      /* 定时发送UI数据 */
      if (osKernelGetTickCount() > tick) {
        tick += delay_tick;
        /* 打包裁判系统数据 */
        Referee_PackCap(&for_cap, &ref);
        Referee_PackAI(&for_ai, &ref);
        Referee_PackLauncher(&for_launcher, &ref);
        Referee_PackChassis(&for_chassis, &ref);

        /* 发送数据到其他进程 */
        osMessageQueueReset(task_runtime.msgq.referee.cap);
        osMessageQueuePut(task_runtime.msgq.referee.cap, &for_cap, 0, 0);
        osMessageQueueReset(task_runtime.msgq.referee.ai);
        osMessageQueuePut(task_runtime.msgq.referee.ai, &for_ai, 0, 0);
        osMessageQueueReset(task_runtime.msgq.referee.chassis);
        osMessageQueuePut(task_runtime.msgq.referee.chassis, &for_chassis, 0,
                          0);
        osMessageQueueReset(task_runtime.msgq.referee.launcher);
        osMessageQueuePut(task_runtime.msgq.referee.launcher, &for_launcher, 0,
                          0);

        /* 获取其他进程数据 */
        osMessageQueueGet(task_runtime.msgq.ui.cap, &(ui.cap_ui), NULL, 0);
        osMessageQueueGet(task_runtime.msgq.ui.chassis, &(ui.chassis_ui), NULL,
                          0);
        osMessageQueueGet(task_runtime.msgq.ui.gimbal, &(ui.gimbal_ui), NULL,
                          0);
        osMessageQueueGet(task_runtime.msgq.ui.launcher, &(ui.launcher_ui),
                          NULL, 0);
        osMessageQueueGet(task_runtime.msgq.ui.cmd, &(ui.cmd_pc), NULL, 0);
        osMessageQueueGet(task_runtime.msgq.ui.ai, &(ui.ai_ui), NULL, 0);

        /* 刷新UI数据 */
        Referee_UIRefresh(&ui);
        Referee_PackUI(&ui, &ref);
      }
    }
  }
}