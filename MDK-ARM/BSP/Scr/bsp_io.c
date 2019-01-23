#include "bsp_io.h"
#include "main.h"

#include "adc.h"
#include "tim.h"

static uint32_t adc_raw;
static Joystick_StatusTypedef js;

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

BSP_StatusTypedef Joystick_Update(Joystick_StatusTypedef* val) {
	if (val == NULL)
		return BSP_FAIL;
	
	HAL_ADC_Start(&hadc1);
	
	if (HAL_ADC_PollForConversion(&hadc1, 1))
		return BSP_FAIL;
	
	adc_raw = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	
	if (adc_raw < 500)
		*val  = JOYSTICK_PRESSED;
	else if (adc_raw < 1000)
		*val = JOYSTICK_LEFT;
	else if (adc_raw < 2000)
		*val = JOYSTICK_RIGHT;
	else if (adc_raw < 3000)
		*val = JOYSTICK_UP;
	else if (adc_raw < 4000)
		*val = JOYSTICK_DOWN;
	else
		*val = JOYSTICK_MID;
	
	return BSP_OK;
}

BSP_StatusTypedef Joystick_WaitInput(void) {
	do {
		BSP_Delay(20);
		Joystick_Update(&js);
	} while (js == JOYSTICK_MID);
	return BSP_OK;
}

BSP_StatusTypedef Joystick_WaitNoInput(void) {
	do {
		BSP_Delay(20);
		Joystick_Update(&js);
	} while (js != JOYSTICK_MID);
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

BSP_StatusTypedef PWM_Set(PWM_NumTypedef n, uint16_t pulse) {
	if (pulse > PWM_RESOLUTION)
		return BSP_FAIL;
	
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
		default: return BSP_FAIL;
	}
	return BSP_OK;
}

BSP_StatusTypedef Power_On(Power_PortTypedef port) {
	switch (port) {
		case POWER_PORT1:
			HAL_GPIO_WritePin(POWER1_CTRL_GPIO_Port, POWER1_CTRL_Pin, GPIO_PIN_RESET);
		break;
		
		case POWER_PORT2:
			HAL_GPIO_WritePin(POWER2_CTRL_GPIO_Port, POWER2_CTRL_Pin, GPIO_PIN_RESET);
		break;
		
		case POWER_PORT3:
			HAL_GPIO_WritePin(POWER3_CTRL_GPIO_Port, POWER3_CTRL_Pin, GPIO_PIN_RESET);
		break;
		
		case POWER_PORT4:
			HAL_GPIO_WritePin(POWER4_CTRL_GPIO_Port, POWER4_CTRL_Pin, GPIO_PIN_RESET);
		break;
	}
	return BSP_OK;
}

BSP_StatusTypedef Power_Off(Power_PortTypedef port) {
	switch (port) {
		case POWER_PORT1:
			HAL_GPIO_WritePin(POWER1_CTRL_GPIO_Port, POWER1_CTRL_Pin, GPIO_PIN_SET);
		break;
		
		case POWER_PORT2:
			HAL_GPIO_WritePin(POWER2_CTRL_GPIO_Port, POWER2_CTRL_Pin, GPIO_PIN_SET);
		break;
		
		case POWER_PORT3:
			HAL_GPIO_WritePin(POWER3_CTRL_GPIO_Port, POWER3_CTRL_Pin, GPIO_PIN_SET);
		break;
		
		case POWER_PORT4:
			HAL_GPIO_WritePin(POWER4_CTRL_GPIO_Port, POWER4_CTRL_Pin, GPIO_PIN_SET);
		break;
	}
	return BSP_OK;
}


BSP_StatusTypedef Laser_On(void) {
	HAL_GPIO_WritePin(LASER_GPIO_Port, LASER_Pin, GPIO_PIN_SET);
	return BSP_OK;
}

BSP_StatusTypedef Laser_Off(void) {
	HAL_GPIO_WritePin(LASER_GPIO_Port, LASER_Pin, GPIO_PIN_RESET);
	return BSP_OK;
}

BSP_StatusTypedef Friction_On(uint16_t pulse) {
	if (pulse > PWM_RESOLUTION)
		return BSP_FAIL;
	
	htim1.Instance->CCR1 = pulse;
	htim1.Instance->CCR4 = pulse;
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
	return BSP_OK;
}

BSP_StatusTypedef Friction_Off(void) {
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
	return BSP_OK;
}

BSP_StatusTypedef Buzzer_On(uint16_t pulse) {
	if (pulse > PWM_RESOLUTION)
		return BSP_FAIL;
	
	htim12.Instance->CCR1 = pulse;
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
	return BSP_OK;
}

BSP_StatusTypedef Buzzer_Off(void) {
	HAL_TIM_PWM_Stop(&htim12, TIM_CHANNEL_1);
	return BSP_OK;
}
