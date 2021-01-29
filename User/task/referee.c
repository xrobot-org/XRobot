/*
  裁判系统任务。

  负责裁判系统的接收和发送。
*/

/* Includes ----------------------------------------------------------------- */
#include "device\referee.h"

#include "bsp\usb.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
#ifdef DEBUG
Referee_t ref;
Referee_UI_t ui;
CMD_UI_t ref_cmd;
#else
static Referee_t ref;
static Referee_UI_t ui;
static CMD_UI_t ref_cmd;
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

  uint32_t last_online_tick = osKernelGetTickCount();

  osDelay(TASK_INIT_DELAY_REFEREE); /* 延时一段时间再开启任务 */

  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_REFEREE;

  /* 初始化裁判系统 */
  Referee_Init(&ref, osThreadGetId());

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.referee = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    Referee_StartReceiving(&ref);
    if (osThreadFlagsWait(SIGNAL_REFEREE_RAW_REDY, osFlagsWaitAll, 0) !=
        SIGNAL_REFEREE_RAW_REDY) {
      if (osKernelGetTickCount() - last_online_tick > 500)
        Referee_HandleOffline(&ref);
    } else {
      Referee_Parse(&ref);
      last_online_tick = osKernelGetTickCount();
    }
    osMessageQueueReset(task_runtime.msgq.referee);
    osMessageQueueReset(task_runtime.msgq.ai.referee);
    osMessageQueuePut(task_runtime.msgq.referee, &(ref), 0, 0);
    osMessageQueuePut(task_runtime.msgq.ai.referee, &(ref), 0, 0);

    while (osMessageQueueGet(task_runtime.msgq.cmd.referee, &ref_cmd, NULL,
                             0) == osOK)
      Referee_PraseCmd(&ui, ref_cmd);
    if (ui.character_counter != 0 || ui.grapic_counter != 0)
      Referee_PackUI(&ui, &ref);

    osDelayUntil(tick);
  }
}
