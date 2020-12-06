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
Referee_t referee;
#else
static CAN_CapOutput_t cap_out;
static Referee_t referee;
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

  osMessageQueueGet(task_runtime.msgq.can.feedback.cap, &can, NULL,
                    osWaitForever);

  uint32_t tick = osKernelGetTickCount();
  uint32_t wakeup = HAL_GetTick();
  while (1) {
#ifdef DEBUG
    task_runtime.stack_water_mark.cap = osThreadGetStackSpace(NULL);
#endif
    tick += delay_tick;

    if (osMessageQueueGet(task_runtime.msgq.can.feedback.cap, &can, NULL,
                          delay_tick) != osOK) {
      CAN_ResetCapOut(&cap_out);
      osMessageQueuePut(task_runtime.msgq.can.output.cap, &cap_out, 0, 0);
    } else {
      osKernelLock();
      const uint32_t now = HAL_GetTick();

      osMessageQueueGet(task_runtime.msgq.referee, &referee, 0, 0);

      Cap_Control(&can.cap, &referee, &cap_out);
      osMessageQueuePut(task_runtime.msgq.can.output.cap, &cap_out, 0, 0);

      wakeup = now;
      osKernelUnlock();

      osDelayUntil(tick);
    }
  }
}
