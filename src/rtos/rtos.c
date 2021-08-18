/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

/* TIM7 are used to generater high freq tick for debug. */
volatile unsigned long high_freq_timer_ticks;

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
/* Code inside this function should be simple and small. */
void configureTimerForRunTimeStats(void) {
  high_freq_timer_ticks = 0;
  // HAL_TIM_Base_Start_IT(&htim7);
}

/* High freq timer ticks for runtime stats */
unsigned long getRunTimeCounterValue(void) { return high_freq_timer_ticks; }

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
  called if a stack overflow is detected. */
  (void)xTask;
  (void)pcTaskName;
  while (1) {
  }
}
