/* Includes ------------------------------------------------------------------*/
#include "bsp_power.h"
#include "gpio.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int Power_On(BSP_Power_Port_t port) {
	switch (port) {
		case POWER_PORT1:
			HAL_GPIO_WritePin(POWER1_CTRL_GPIO_Port, POWER1_CTRL_Pin, GPIO_PIN_RESET);
		break;
		
		case POWER_PORT2:
			HAL_GPIO_WritePin(POWER2_CTRL_GPIO_Port, POWER2_CTRL_Pin, GPIO_PIN_RESET);
		break;
		
		case POWER_PORT3:
			HAL_GPIO_WritePin(POWER3_CTRL_GPIO_Port, POWER3_CTRL_Pin, GPIO_PIN_RESET);
		break;
		
		case POWER_PORT4:
			HAL_GPIO_WritePin(POWER4_CTRL_GPIO_Port, POWER4_CTRL_Pin, GPIO_PIN_RESET);
		break;
	}
	return 0;
}

int Power_Off(BSP_Power_Port_t port) {
	switch (port) {
		case POWER_PORT1:
			HAL_GPIO_WritePin(POWER1_CTRL_GPIO_Port, POWER1_CTRL_Pin, GPIO_PIN_SET);
		break;
		
		case POWER_PORT2:
			HAL_GPIO_WritePin(POWER2_CTRL_GPIO_Port, POWER2_CTRL_Pin, GPIO_PIN_SET);
		break;
		
		case POWER_PORT3:
			HAL_GPIO_WritePin(POWER3_CTRL_GPIO_Port, POWER3_CTRL_Pin, GPIO_PIN_SET);
		break;
		
		case POWER_PORT4:
			HAL_GPIO_WritePin(POWER4_CTRL_GPIO_Port, POWER4_CTRL_Pin, GPIO_PIN_SET);
		break;
	}
	return 0;
}



