#include "pwm.h"
#include "main.h"

#include "tim.h"

Board_Status_t PWM_Start(PWM_Num_t n) {
	switch(n) {
		case PWM_A: HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_4); break;
		case PWM_B: HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3); break;
		case PWM_C: HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2); break;
		case PWM_D: HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1); break;
		case PWM_E: HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); break;
		case PWM_F: HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3); break;
		case PWM_G: HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2); break;
		case PWM_H: HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); break;
		case PWM_S: HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); break;
		case PWM_T: HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2); break;
		case PWM_U: HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3); break;
		case PWM_V: HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4); break;
		case PWM_W:	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1); break;
		case PWM_X:	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2); break;
		case PWM_Y:	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3); break;
		case PWM_Z:	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4); break;
		case PWM_IMU_HEAT: HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); break;
	}
	return BOARD_OK;
}

Board_Status_t PWM_Set(PWM_Num_t n, uint16_t pulse) {
	if (pulse > PWM_RESOLUTION)
		return BOARD_FAIL;
	
	switch(n) {
		case PWM_A: htim5.Instance->CCR4 = pulse; break;
		case PWM_B: htim5.Instance->CCR3 = pulse; break;
		case PWM_C: htim5.Instance->CCR2 = pulse; break;
		case PWM_D: htim5.Instance->CCR1 = pulse; break;
		case PWM_E: htim4.Instance->CCR4 = pulse; break;
		case PWM_F: htim4.Instance->CCR3 = pulse; break;
		case PWM_G: htim4.Instance->CCR2 = pulse; break;
		case PWM_H: htim4.Instance->CCR1 = pulse; break;
		case PWM_S: htim2.Instance->CCR1 = pulse; break;
		case PWM_T: htim2.Instance->CCR2 = pulse; break;
		case PWM_U: htim2.Instance->CCR3 = pulse; break;
		case PWM_V: htim2.Instance->CCR4 = pulse; break;
		case PWM_W:	htim8.Instance->CCR1 = pulse; break;
		case PWM_X:	htim8.Instance->CCR2 = pulse; break;
		case PWM_Y:	htim8.Instance->CCR3 = pulse; break;
		case PWM_Z:	htim8.Instance->CCR4 = pulse; break;
		case PWM_IMU_HEAT: htim3.Instance->CCR2 = pulse; break;
		default: return BOARD_FAIL;
	}
	return BOARD_OK;
}

Board_Status_t Friction_On(uint16_t pulse) {
	if (pulse > PWM_RESOLUTION)
		return BOARD_FAIL;
	
	htim1.Instance->CCR1 = pulse;
	htim1.Instance->CCR4 = pulse;
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	Board_Delay(500);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	return BOARD_OK;
}

Board_Status_t Friction_Off(void) {
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
	return BOARD_OK;
}
