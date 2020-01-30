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

int BSP_Buzzer_Set(float freq) {
	uint16_t pulse = freq * PWM_RESOLUTION;
	htim12.Instance->CCR4 = pulse;
	return 0;
}
int BSP_Buzzer_Stop(void) {
	return HAL_TIM_PWM_Stop(&htim12, TIM_CHANNEL_4);
}
