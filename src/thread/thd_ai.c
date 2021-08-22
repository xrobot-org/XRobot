/*
  AI上位机通信任务
*/

/* Includes ----------------------------------------------------------------- */
#include "dev_ai.h"
#include "thd.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
#ifdef MCU_DEBUG_BUILD
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
void Thread_AI(void *argument) {
  UNUSED(argument); /* 未使用argument，消除警告 */

  /* 计算任务运行到指定频率需要等待的tick数 */
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_AI;

  /* 初始化AI通信 */
  AI_Init(&ai);

  uint32_t tick = osKernelGetTickCount();

  uint32_t last_online_tick = tick;

  while (1) {
#ifdef MCU_DEBUG_BUILD
    runtime.stack_water_mark.ai = osThreadGetStackSpace(osThreadGetId());
#endif
    /* Task body */
    tick += delay_tick;

    xQueueReceive(runtime.msgq.ai.quat, &(ai_quat), 0);
    xQueueReceive(runtime.msgq.cmd.ai, &(ai.mode), 0);
    bool ref_update =
        (xQueueReceive(runtime.msgq.referee.ai, &(referee_ai), 0) == pdPASS);

    AI_StartReceiving(&ai);
    if (AI_WaitDmaCplt()) {
      AI_ParseHost(&ai);
      last_online_tick = tick;
    } else {
      if (tick - last_online_tick > 300) AI_HandleOffline(&ai);
    }

    if (ai.mode != AI_MODE_STOP && ai.ai_online) {
      AI_PackCmd(&ai, &cmd_host);
      xQueueOverwrite(runtime.msgq.cmd.src.host, &(cmd_host));
    }

    AI_PackMcu(&ai, &ai_quat);
    if (ref_update) AI_PackRef(&ai, &(referee_ai));

    AI_StartTrans(&ai, ref_update);

    AI_PackUi(&ai_ui, &ai);
    xQueueOverwrite(runtime.msgq.ui.ai, &(cmd_host));
    osDelayUntil(tick);
  }
}
