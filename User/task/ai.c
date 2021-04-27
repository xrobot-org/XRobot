/*
  AI上位机通信任务
*/

/* Includes ----------------------------------------------------------------- */
#include "device/ai.h"

#include "bsp/usb.h"
#include "task/user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
#ifdef DEBUG
AI_t ai;
CMD_Host_t cmd_host;
AHRS_Quaternion_t quat;
Referee_ForAI_t referee_ai;
#else
static AI_t ai;
static CMD_Host_t cmd_host;
static AHRS_Quaternion_t quat;
static Referee_ForAI_t referee_ai;
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

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_AI;

  /* 初始化AI通信 */
  AI_Init(&ai);

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.ai = osThreadGetStackSpace(osThreadGetId());
#endif
    /* Task body */
    tick += delay_tick;

    AI_StartReceiving(&ai);
    if (AI_WaitDmaCplt()) {
      AI_ParseHost(&ai, &cmd_host);
    } else {
      AI_HandleOffline(&ai, &cmd_host);
    }
    osMessageQueueReset(task_runtime.msgq.cmd.raw.host);
    osMessageQueuePut(task_runtime.msgq.cmd.raw.host, &(cmd_host), 0, 0);

    osMessageQueueGet(task_runtime.msgq.ai.quat, &(quat), NULL, 0);
    osMessageQueueGet(task_runtime.msgq.cmd.ai, &(ai.status), NULL, 0);
    bool ref_update = (osMessageQueueGet(task_runtime.msgq.referee.ai,
                                         &(referee_ai), NULL, 0) == osOK);
    AI_PackMcu(&ai, &quat);
    if (ref_update) AI_PackRef(&ai, &(referee_ai));

    AI_StartTrans(&(ai), ref_update);

    AI_PackUi(&ai);
    osMessageQueueReset(task_runtime.msgq.ui.ai);
    osMessageQueuePut(task_runtime.msgq.ui.ai, &(ai.ui), 0, 0);

    osDelayUntil(tick);
  }
}
