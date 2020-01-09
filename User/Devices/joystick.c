#include "joystick.h"

#include "main.h"
#include "adc.h"

static uint32_t adc_raw;
static Joystick_Status_t js;

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

