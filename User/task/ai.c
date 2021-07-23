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
AI_UI_t ai_ui;
CMD_Host_t cmd_host;
AHRS_Quaternion_t ai_quat;
Referee_ForAI_t referee_ai;
#else
static AI_t ai;
static AI_UI_t ai_ui;
static CMD_Host_t cmd_host;
static AHRS_Quaternion_t ai_quat;
static Referee_ForAI_t referee_ai;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * @brief AI通信
 *
 * @param argument 未使用
 */
void Task_Ai(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_AI;

  /* 初始化AI通信 */
  AI_Init(&ai);

  uint32_t tick = osKernelGetTickCount();

  uint32_t last_online_tick = tick;

  while (1) {
#ifdef DEBUG
    runtime.stack_water_mark.ai = osThreadGetStackSpace(osThreadGetId());
#endif
    /* Task body */
    tick += delay_tick;

    osMessageQueueGet(runtime.msgq.ai.quat, &(ai_quat), NULL, 0);
    osMessageQueueGet(runtime.msgq.cmd.ai, &(ai.mode), NULL, 0);
    bool ref_update = (osMessageQueueGet(runtime.msgq.referee.ai, &(referee_ai),
                                         NULL, 0) == osOK);

    AI_StartReceiving(&ai);
    if (AI_WaitDmaCplt()) {
      AI_ParseHost(&ai);
      last_online_tick = tick;
    } else {
      if (tick - last_online_tick > 300) AI_HandleOffline(&ai);
    }

    if (ai.mode != AI_MODE_STOP && ai.ai_online) {
      AI_PackCmd(&ai, &cmd_host);
      osMessageQueueReset(runtime.msgq.cmd.src.host);
      osMessageQueuePut(runtime.msgq.cmd.src.host, &(cmd_host), 0, 0);
    }

    AI_PackMcu(&ai, &ai_quat);
    if (ref_update) AI_PackRef(&ai, &(referee_ai));

    AI_StartTrans(&ai, ref_update);

    AI_PackUi(&ai_ui, &ai);
    osMessageQueueReset(runtime.msgq.ui.ai);
    osMessageQueuePut(runtime.msgq.ui.ai, &ai_ui, 0, 0);

    osDelayUntil(tick);
  }
}
