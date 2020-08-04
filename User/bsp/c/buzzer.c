/* Includes ------------------------------------------------------------------*/
#include "bsp\buzzer.h"

#include <main.h>
#include <tim.h>

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int8_t BSP_Buzzer_Start(void) {
	if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3) == HAL_OK)
		return BSP_OK;
	return BSP_ERR;
}

int8_t BSP_Buzzer_Set(float freq, float duty_cycle) {
	uint16_t pulse = (uint16_t)(duty_cycle * (float)UINT16_MAX);
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, pulse);
	
	pulse = (uint16_t)freq;
	__HAL_TIM_PRESCALER(&htim4, pulse);
	return BSP_OK;
}

int8_t BSP_Buzzer_Stop(void) {
	if (HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3) == HAL_OK)
		return BSP_OK;
	return BSP_ERR;
}
