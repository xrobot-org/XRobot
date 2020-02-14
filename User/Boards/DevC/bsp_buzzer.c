/* Includes ------------------------------------------------------------------*/
#include "bsp_buzzer.h"

#include "main.h"
#include "tim.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int BSP_Buzzer_Start(void) {
	return HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3); 
}

int BSP_Buzzer_Set(float freq, float duty_cycle) {
	uint16_t pulse = duty_cycle * UINT16_MAX;
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, pulse);
	
	pulse = freq;
	__HAL_TIM_PRESCALER(&htim4, pulse);
	return 0;
}

int BSP_Buzzer_Stop(void) {
	return HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);
}
