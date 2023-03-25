#include "bsp_delay.h"

#include "stm32f1xx_hal.h"

int8_t bsp_delay(uint32_t ms) {
  HAL_Delay(ms);

  return BSP_OK;
}
