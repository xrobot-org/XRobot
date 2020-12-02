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
Ai_t ai;
#else
static Ai_t ai;
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

  osDelay(TASK_INIT_DELAY_AI); /* 延时一段时间再开启 */

  /* 初始化AI通信 */
  Ai_Init(&ai, osThreadGetId());

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.ai = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    Ai_StartReceiving(&ai);
    if (AI_WaitDmaCplt()) {
      Ai_Parse(&ai);
      osMessageQueuePut(task_runtime.msgq.raw_cmd.ai_raw, &(ai.command), 0, 0);
    }

    osDelayUntil(tick);
  }
}
