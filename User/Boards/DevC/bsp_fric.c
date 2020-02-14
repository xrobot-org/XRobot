/* Includes ------------------------------------------------------------------*/
#include "bsp_fric.h"
#include "bsp_delay.h"

#include "tim.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int BSP_Fric_Start(void) {
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	BSP_Delay(500);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	return 0;
}
int BSP_Fric_Set(float duty_cycle) {
	uint16_t pulse = duty_cycle * UINT16_MAX;
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, pulse);
	return 0;
}

int BSP_Fric_Stop(void) {
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
	return 0;
}
