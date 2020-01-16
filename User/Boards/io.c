#include "io.h"
#include "main.h"

#include "adc.h"
#include "tim.h"


Board_Status_t LED_Set(LED_Num_t n, LED_Status_t s) {
	GPIO_TypeDef* gpiox;
	uint16_t gpio_pin;

	switch (n) {
#ifdef USE_DEV_BOARD_C
		case LED_BLU:
			gpio_pin = LED3_Pin;
			gpiox = LED3_GPIO_Port;
		break;
		
#elif defined USE_DEV_BOARD_A
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
#endif
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
	
	return BOARD_OK;
}

Board_Status_t Power_On(Power_Port_t port) {
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
	return BOARD_OK;
}

Board_Status_t Power_Off(Power_Port_t port) {
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
	return BOARD_OK;
}


Board_Status_t Laser_On(void) {
	HAL_GPIO_WritePin(LASER_GPIO_Port, LASER_Pin, GPIO_PIN_SET);
	return BOARD_OK;
}

Board_Status_t Laser_Off(void) {
	HAL_GPIO_WritePin(LASER_GPIO_Port, LASER_Pin, GPIO_PIN_RESET);
	return BOARD_OK;
}

Board_Status_t Buzzer_On(uint16_t pulse) {
	if (pulse > PWM_RESOLUTION)
		return BOARD_FAIL;
	
#ifdef STM32F407xx
	htim4.Instance->CCR1 = pulse;
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
		
#elif defined STM32F427xx
	htim12.Instance->CCR1 = pulse;
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
	
#endif
	
	return BOARD_OK;
}

Board_Status_t Buzzer_Off(void) {
#ifdef STM32F407xx
	HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
		
#elif defined STM32F427xx
	HAL_TIM_PWM_Stop(&htim12, TIM_CHANNEL_1);
	
#endif
	
	return BOARD_OK;
}
