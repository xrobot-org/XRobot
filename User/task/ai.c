/*
  AI上位机通信任务
*/

/* Includes ----------------------------------------------------------------- */
#include "device\ai.h"

#include "bsp\usb.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
#ifdef DEBUG
AI_t ai;
CMD_Host_t cmd_host;
#else
static AI_t ai;
CMD_Host_t cmd_host;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief AI通信
 *
 * \param argument 未使用
 */
void Task_Ai(void *argument) {
  (void)argument; /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率，需要延时的时间 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_AI;

  /* 初始化AI通信 */
  AI_Init(&ai, osThreadGetId());

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.ai = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    AI_StartReceiving(&ai);

    if (AI_WaitDmaCplt()) {
      AI_ParseHost(&ai, &cmd_host);
    } else {
      AI_HandleOffline(&ai, &cmd_host);
    }

    osMessageQueuePut(task_runtime.msgq.cmd.raw.host, &(cmd_host), 0, 0);

    osDelayUntil(tick);
  }
}
