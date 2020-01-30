/* Includes ------------------------------------------------------------------*/
#include "bsp_fric.h"
#include "tim.h"

#include "bsp_delay.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int Friction_On(uint16_t pulse) {
	if (pulse > PWM_RESOLUTION)
		return -1;

	htim1.Instance->CCR1 = pulse;
	htim1.Instance->CCR4 = pulse;
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	BSP_Delay(500);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	
	return 0;
}

int Friction_Off(void) {
#ifdef STM32F407xx
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
		
#elif defined STM32F427xx
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);

#endif
	
	return 0;
}
