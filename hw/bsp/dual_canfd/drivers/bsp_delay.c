#include "bsp_delay.h"

#include "main.h"

bsp_status_t bsp_delay(uint32_t ms) {
  HAL_Delay(ms);

  return BSP_OK;
}
