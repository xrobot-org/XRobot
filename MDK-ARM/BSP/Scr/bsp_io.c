#include "bsp_io.h"
#include "main.h"

#include "adc.h"
#include "tim.h"

BSP_StatusTypedef LED_Set(LED_NumTypedef n, LED_StatusTypedef s) {
	GPIO_TypeDef* gpiox;
	uint16_t gpio_pin;
	
	switch (n) {
		case LED1:
			gpio_pin = LED1_Pin;
			gpiox = LED1_GPIO_Port;
		break;
		
		case LED2:
			gpio_pin = LED2_Pin;
			gpiox = LED2_GPIO_Port;
		break;
		
		case LED3:
			gpio_pin = LED3_Pin;
			gpiox = LED3_GPIO_Port;
		break;
		
		case LED4:
			gpio_pin = LED4_Pin;
			gpiox = LED4_GPIO_Port;
		break;
		
		case LED5:
			gpio_pin = LED5_Pin;
			gpiox = LED5_GPIO_Port;
		break;
		
		case LED6:
			gpio_pin = LED6_Pin;
			gpiox = LED6_GPIO_Port;
		break;
		
		case LED7:
			gpio_pin = LED7_Pin;
			gpiox = LED7_GPIO_Port;
		break;
		
		case LED8:
			gpio_pin = LED8_Pin;
			gpiox = LED8_GPIO_Port;
		break;
			
		case LED_RED:
			gpiox = LED_RED_GPIO_Port;
			gpio_pin = LED_RED_Pin ;
		break;
		
		case LED_GRN:
			gpiox = LED_GRN_GPIO_Port;
			gpio_pin = LED_GRN_Pin ;
		break;
	}
	
	switch (s) {
		case LED_ON:
			HAL_GPIO_WritePin(gpiox, gpio_pin, GPIO_PIN_RESET);
		break;
		
		case LED_OFF:
			HAL_GPIO_WritePin(gpiox, gpio_pin, GPIO_PIN_SET);
		break;
		
		case LED_TAGGLE:
			HAL_GPIO_TogglePin(gpiox, gpio_pin);
		break;
	}
	
	return BSP_OK;
}

BSP_StatusTypedef Joystick_Update(Joystick_HandleTypedef *hjs) {
	HAL_ADC_Start(&hadc1);
	
	if (HAL_ADC_PollForConversion(&hadc1, 1))
		return BSP_FAIL;
	
	hjs->raw = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	
	if (hjs->raw < 500)
		hjs->status = JOYSTICK_PRESSED;
	else if (hjs->raw < 1000)
		hjs->status = JOYSTICK_LEFT;
	else if (hjs->raw < 2000)
		hjs->status = JOYSTICK_RIGHT;
	else if (hjs->raw < 3000)
		hjs->status = JOYSTICK_UP;
	else if (hjs->raw < 4000)
		hjs->status = JOYSTICK_DOWN;
	else
		hjs->status = JOYSTICK_MID;
	return BSP_OK;
}

BSP_StatusTypedef PWM_Start(PWM_NumTypedef n) {
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
		default: return BSP_FAIL;
	}
	return BSP_OK;
}

BSP_StatusTypedef PWM_Set(PWM_NumTypedef n, float duty_cycle) {
	switch(n) {
		case PWM_A: htim5.Instance->CCR4 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_B: htim5.Instance->CCR3 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_C: htim5.Instance->CCR2 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_D: htim5.Instance->CCR1 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_E: htim4.Instance->CCR4 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_F: htim4.Instance->CCR3 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_G: htim4.Instance->CCR2 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_H: htim4.Instance->CCR1 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_S: htim2.Instance->CCR1 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_T: htim2.Instance->CCR2 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_U: htim2.Instance->CCR3 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_V: htim2.Instance->CCR4 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_W:	htim8.Instance->CCR1 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_X:	htim8.Instance->CCR2 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_Y:	htim8.Instance->CCR3 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_Z:	htim8.Instance->CCR4 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		case PWM_IMU_HEAT: htim3.Instance->CCR2 = (PWM_RESOLUTION * duty_cycle) - 1; break;
		default: return BSP_FAIL;
	}
	return BSP_OK;
}

BSP_StatusTypedef Power_Set(Power_PortTypedef port ,Power_StatusTypedef state) {
	GPIO_TypeDef* gpiox;
	uint16_t gpio_pin;
	GPIO_PinState s;
	
	switch (port) {
		case POWER_PORT1:
			gpiox = POWER1_CTRL_GPIO_Port;
			gpio_pin = POWER1_CTRL_Pin;
		break;
		
		case POWER_PORT2:
			gpiox = POWER2_CTRL_GPIO_Port;
			gpio_pin = POWER2_CTRL_Pin;
		break;
		
		case POWER_PORT3:
			gpiox = POWER3_CTRL_GPIO_Port;
			gpio_pin = POWER3_CTRL_Pin;
		break;
		
		case POWER_PORT4:
			gpiox = POWER4_CTRL_GPIO_Port;
			gpio_pin = POWER4_CTRL_Pin;
		break;
	}
	
	switch (s) {
		case POWER_ON:
			s = GPIO_PIN_RESET;
		break;
		
		case POWER_OFF:
			s = GPIO_PIN_SET;
		break;
	}
	HAL_GPIO_WritePin(gpiox, gpio_pin, s);
	
	return BSP_OK;
}
