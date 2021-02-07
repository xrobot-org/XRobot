/*
 */

/* Includes ----------------------------------------------------------------- */
#include "module\cap.h"

#include "device\referee.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */

/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static CAN_t can;

#ifdef DEBUG
CAN_CapOutput_t cap_out;
Referee_ForCap_t referee_cap;
#else
static CAN_CapOutput_t cap_out;
static Referee_ForCap_t referee_cap;
#endif

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

/**
 * \brief Capacity controller
 *
 * \param argument unused
 */
void Task_Cap(void *argument) {
  (void)argument;

  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_CTRL_CAP;

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.cap = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick;

    osMessageQueueGet(task_runtime.msgq.referee.cap, &referee_cap, 0, 0);
    if (osMessageQueueGet(task_runtime.msgq.can.feedback.cap, &can, NULL,
                          delay_tick) != osOK) {
      CAN_CAP_HandleOffline(&(can.cap), &cap_out,
                            referee_cap.chassis_power_limit);
      osMessageQueuePut(task_runtime.msgq.can.output.cap, &cap_out, 0, 0);
      osMessageQueuePut(task_runtime.msgq.cap_info, &(can.cap), 0, 0);
    } else {
      osKernelLock();
      Cap_Control(&can.cap, &referee_cap, &cap_out);
      osKernelUnlock();
      osMessageQueuePut(task_runtime.msgq.can.output.cap, &cap_out, 0, 0);
      osMessageQueuePut(task_runtime.msgq.cap_info, &(can.cap), 0, 0);

      osDelayUntil(tick);
    }
  }
}
