/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"

#include "bsp_delay.h"

#include "main.h"

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
		case osKernelRunning:
			osDelay(ticks ? ticks : 1);
			break;
		
		case osKernelInactive:
		case osKernelReady:
		case osKernelLocked:
		case osKernelSuspended:
		case osKernelError:
		case osKernelReserved:
			HAL_Delay(ms);
			break;
	}
	return 0;
}
