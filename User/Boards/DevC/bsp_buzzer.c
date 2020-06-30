/* Includes ------------------------------------------------------------------*/
#include "bsp_buzzer.h"

#include "main.h"
#include "tim.h"

#include "user_math.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int8_t BSP_Buzzer_Start(void) {
	return HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3); 
}

int8_t BSP_Buzzer_Set(float32_t freq, float32_t duty_cycle) {
	uint16_t pulse = duty_cycle * UINT16_MAX;
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, pulse);
	
	pulse = freq;
	__HAL_TIM_PRESCALER(&htim4, pulse);
	return 0;
}

int8_t BSP_Buzzer_Stop(void) {
	return HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3);
}
