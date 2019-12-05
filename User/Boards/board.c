#include "board.h"
#include "main.h"
#include "cmsis_os.h"

void BoardDelay(uint32_t ms) {
  if (osKernelGetState() == osKernelRunning)
		osDelay(ms);
	else
		HAL_Delay(ms);
}
