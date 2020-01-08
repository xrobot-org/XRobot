#include "io.h"
#include "main.h"

#include "adc.h"
#include "tim.h"

static uint32_t adc_raw;
static Joystick_Status_t js;

Board_Status_t LED_Set(LED_Num_t n, LED_Status_t s) {
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
	
	return BOARD_OK;
}

Board_Status_t Joystick_Update(Joystick_Status_t* val) {
	if (val == NULL)
		return BOARD_FAIL;
	
	HAL_ADC_Start(&hadc1);
	
	if (HAL_ADC_PollForConversion(&hadc1, 1))
		return BOARD_FAIL;
	
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
	
	return BOARD_OK;
}

Board_Status_t Joystick_WaitInput(void) {
	do {
		Board_Delay(20);
		Joystick_Update(&js);
	} while (js == JOYSTICK_MID);
	return BOARD_OK;
}

Board_Status_t Joystick_WaitNoInput(void) {
	do {
		Board_Delay(20);
		Joystick_Update(&js);
	} while (js != JOYSTICK_MID);
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
	
	htim12.Instance->CCR1 = pulse;
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
	return BOARD_OK;
}

Board_Status_t Buzzer_Off(void) {
	HAL_TIM_PWM_Stop(&htim12, TIM_CHANNEL_1);
	return BOARD_OK;
}
