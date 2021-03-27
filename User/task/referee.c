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
Referee_ForCap_t for_cap;
Referee_ForAI_t for_ai;
Referee_ForChassis_t for_chassis;
Referee_ForShoot_t for_shoot;
#else
static Referee_t ref;
static Referee_UI_t ui;
static CMD_UI_t ref_cmd;
static Referee_ForCap_t for_cap;
static Referee_ForAI_t for_ai;
static Referee_ForChassis_t for_chassis;
static Referee_ForShoot_t for_shoot;
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

    Referee_StartReceiving(&ref);
    if (osThreadFlagsWait(SIGNAL_REFEREE_RAW_REDY, osFlagsWaitAll, 10) !=
        SIGNAL_REFEREE_RAW_REDY) {
      if (osKernelGetTickCount() - last_online_tick > 500)
        Referee_HandleOffline(&ref);
    } else {
      Referee_Parse(&ref);
      last_online_tick = osKernelGetTickCount();
    }
    Referee_PackCap(&(for_cap), (const Referee_t *)&ref);
    Referee_PackAI(&(for_ai), (const Referee_t *)&ref);
    Referee_PackShoot(&(for_shoot), (const Referee_t *)&ref);
    Referee_PackChassis(&(for_chassis), (const Referee_t *)&ref);
    if (osKernelGetTickCount() > delay_tick) {
      tick += delay_tick;
      osMessageQueueReset(task_runtime.msgq.referee.cap);
      osMessageQueueReset(task_runtime.msgq.referee.ai);
      osMessageQueueReset(task_runtime.msgq.referee.chassis);
      osMessageQueueReset(task_runtime.msgq.referee.shoot);
      osMessageQueuePut(task_runtime.msgq.referee.cap, &(for_cap), 0, 0);
      osMessageQueuePut(task_runtime.msgq.referee.ai, &(for_ai), 0, 0);
      osMessageQueuePut(task_runtime.msgq.referee.chassis, &(for_chassis), 0,
                        0);
      osMessageQueuePut(task_runtime.msgq.referee.shoot, &(for_shoot), 0, 0);
    }

    while (osMessageQueueGet(task_runtime.msgq.cmd.referee, &ref_cmd, NULL,
                             0) == osOK)
      Referee_PraseCmd(&ui, ref_cmd);
    if (ui.character_counter != 0 || ui.grapic_counter != 0)
      Referee_PackUI(&ui, &ref);
  }
}
