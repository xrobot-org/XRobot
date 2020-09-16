/*
  裁判系统任务。
  
  负责裁判系统的接收和发送。
*/

/* Includes ------------------------------------------------------------------*/
#include "device\referee.h"

#include "bsp\usb.h"
#include "task\user_task.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#ifdef DEBUG
Referee_t ref;
#else
static Referee_t ref;
#endif

/* Private function ----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Referee(void *argument) {
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_REFEREE;
  Task_Param_t *task_param = (Task_Param_t *)argument;

  /* Task Setup */
  osDelay(TASK_INIT_DELAY_REFEREE);

  Referee_Init(&ref, osThreadGetId());

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_param->stack_water_mark.referee = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    Referee_StartReceiving(&ref);
    osThreadFlagsWait(SIGNAL_REFEREE_RAW_REDY, osFlagsWaitAll, osWaitForever);

    Referee_Parse(&ref);

    osDelayUntil(tick);
  }
}
