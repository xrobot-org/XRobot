#include "bsp_delay.h"

#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "task.h"

int8_t bsp_delay(uint32_t ms) {
  switch (xTaskGetSchedulerState()) {
    case taskSCHEDULER_NOT_STARTED:
      HAL_Delay(ms);
      break;

    case taskSCHEDULER_SUSPENDED:
    case taskSCHEDULER_RUNNING:
      vTaskDelay(pdMS_TO_TICKS(ms));
      break;
  }
  return BSP_OK;
}
