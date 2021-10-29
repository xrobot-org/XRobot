#include "hal.h"
#include "thd.h"

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* MCU Configuration */

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  HAL_InitPeripherals();

  /* Init task */
  Thd_Init();

  /* Start scheduler */
  vTaskStartScheduler();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  while (1) {
  }
}
