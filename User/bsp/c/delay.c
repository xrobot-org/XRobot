/* Includes ------------------------------------------------------------------*/
#include "bsp\delay.h"

#include <main.h>
#include <cmsis_os2.h>

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int8_t BSP_Delay(uint32_t ms) {
	uint32_t tick_period = 1000u / osKernelGetTickFreq();
	uint32_t ticks = ms / tick_period;
	
	switch (osKernelGetState()) {
		case osKernelError:
		case osKernelReserved:
		case osKernelLocked:
		case osKernelSuspended:
			return BSP_ERR;
		
		case osKernelRunning:
			osDelay(ticks ? ticks : 1);
			break;
		
		case osKernelInactive:
		case osKernelReady:
			HAL_Delay(ms);
			break;
		
	}
	return BSP_OK;
}
