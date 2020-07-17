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
	return HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_4); 
}

int BSP_Buzzer_Set(float freq, float duty_cycle) {
	uint16_t pulse = freq * PWM_RESOLUTION;
	__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_4, pulse);
	
	pulse = duty_cycle * PWM_RESOLUTION;
	__HAL_TIM_PRESCALER(&htim12, pulse);
	return 0;
}
int BSP_Buzzer_Stop(void) {
	return HAL_TIM_PWM_Stop(&htim12, TIM_CHANNEL_4);
}
