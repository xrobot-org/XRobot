/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"

#include "bsp_delay.h"

#include "main.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int BSP_Delay(uint32_t ms) {
	uint32_t ticks = ms / portTICK_PERIOD_MS;
	
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
