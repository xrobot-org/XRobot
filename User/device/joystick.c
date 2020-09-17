/*
        OLED模块�?带的摇杆�?

*/

/* Includes ----------------------------------------------------------------- */
#include "joystick.h"

/* Include BSP相关的头文件*/
#include <adc.h>

#include "bsp_delay.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
static uint32_t adc_raw;
static Joystick_Status_t js;

int Joystick_Update(Joystick_Status_t *val) {
  if (val == NULL) return -1;

  HAL_ADC_Start(&hadc1);

  if (HAL_ADC_PollForConversion(&hadc1, 1)) return -1;

  adc_raw = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  if (adc_raw < 500)
    *val = JOYSTICK_PRESSED;
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

  return 0;
}

int Joystick_WaitInput(void) {
  do {
    BSP_Delay(20);
    Joystick_Update(&js);
  } while (js == JOYSTICK_MID);
  return 0;
}

int Joystick_WaitNoInput(void) {
  do {
    BSP_Delay(20);
    Joystick_Update(&js);
  } while (js != JOYSTICK_MID);
  return 0;
}
