#include "bsp_common.h"
#include "main.h"
#include "cmsis_os.h"

void BSP_Delay(uint32_t ms) {
  if (osKernelRunning())
		osDelay(ms);
	else
		HAL_Delay(ms);
}
