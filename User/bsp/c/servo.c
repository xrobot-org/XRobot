/* Includes ------------------------------------------------------------------*/
#include "bsp\servo.h"

#include "main.h"
#include "tim.h"

#include "component\user_math.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
float32_t range[BSP_SERVO_NUM];


/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int8_t BSP_Servo_Init(BSP_Servo_Channel_t ch, float32_t max_angle) {
	range[ch] = max_angle;
	
	return 0;
}

int8_t BSP_Servo_Start(BSP_Servo_Channel_t ch) {
	switch(ch) {
		case BSP_SERVO_A: 
			HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); 
			break;
		
		default:
			return -1;
	}
	return 0;
}


int8_t BSP_Servo_Set(BSP_Servo_Channel_t ch, uint8_t angle) {
	if (angle > 1.f)
		return -1;
	
	uint16_t pulse = angle * UINT16_MAX;
	
	switch(ch) {
		case BSP_SERVO_A: 
			__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pulse);
			break;
		
		default:
			return -1;
	}
	return 0;
}

int8_t BSP_Servo_Stop(BSP_Servo_Channel_t ch) {
	switch(ch) {
		case BSP_SERVO_A:
			HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
			break;
		
		default:
			return -1;
	}
	return 0;
}
