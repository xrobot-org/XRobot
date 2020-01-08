#include "board.h"
#include "cmsis_os.h"

Board_Status_t Board_Delay(uint32_t ms) {
  if (osKernelGetState() == osKernelRunning)
		osDelay(ms);
	else
		HAL_Delay(ms);
	
	return BOARD_OK;
}
