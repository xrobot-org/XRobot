/* Includes ------------------------------------------------------------------*/
#include "bsp\pwm.h"

#include <main.h>
#include <tim.h>

#include "component\user_math.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int8_t BSP_PWM_Start(BSP_PWM_Channel_t ch) {
	switch(ch) {
		case BSP_PWM_IMU_HEAT: 
			HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); 
			break;
	}
	return 0;
}

int8_t BSP_PWM_Set(BSP_PWM_Channel_t ch, float duty_cycle) {
	if (duty_cycle > 1.f)
		return -1;
	
	uint16_t pulse = (uint16_t)(duty_cycle * (float)UINT16_MAX);
	
	switch(ch) {
		case BSP_PWM_IMU_HEAT: 
			__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
			break;
	}
	return 0;
}

int8_t BSP_PWM_Stop(BSP_PWM_Channel_t ch) {
	switch(ch) {
		case BSP_PWM_IMU_HEAT:
			HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
			break;
	}
	return 0;
}
