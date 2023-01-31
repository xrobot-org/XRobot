#include "bsp_time.h"

#include "FreeRTOS.h"
#include "main.h"
#include "task.h"

uint32_t bsp_time_get_ms() { return xTaskGetTickCount(); }

uint32_t bsp_time_get_us() {
  return xTaskGetTickCount() * 1000 + __HAL_TIM_GET_COUNTER(&htim14);
}

float bsp_time_get() {
  return (float)((xTaskGetTickCount() * 1000 + __HAL_TIM_GET_COUNTER(&htim14)) /
                 1000000.0f);
}
