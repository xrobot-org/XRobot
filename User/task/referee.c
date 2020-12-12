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
#else
static Referee_t ref;
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

  /* 计算任务运行到指定频率，需要延时的时间 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_REFEREE;

  osDelay(TASK_INIT_DELAY_REFEREE); /* 延时一段时间再开启任务 */

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
    if (osThreadFlagsWait(SIGNAL_REFEREE_RAW_REDY, osFlagsWaitAll, 200) !=
        osOK) {
      Referee_HandleOffline(&ref);
    } else {
      Referee_Parse(&ref);
    }
    osMessageQueueReset(task_runtime.msgq.referee);
    osMessageQueuePut(task_runtime.msgq.referee, &ref, 0, 0);

    osDelayUntil(tick);
  }
}
