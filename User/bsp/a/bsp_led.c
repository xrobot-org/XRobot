/* Includes ----------------------------------------------------------------- */
#include "bsp_led.h"

#include "gpio.h"

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */

uint8_t BSP_LED_Set(BSP_LED_Channel_t ch, BSP_LED_Status_t s, int16_t duty_cycle) {
  GPIO_TypeDef *gpiox;
  uint16_t gpio_pin;

  switch (ch) {
    case BSP_LED1:
      gpio_pin = LED1_Pin;
      gpiox = LED1_GPIO_Port;
      break;

    case BSP_LED2:
      gpio_pin = LED2_Pin;
      gpiox = LED2_GPIO_Port;
      break;

    case BSP_LED3:
      gpio_pin = LED3_Pin;
      gpiox = LED3_GPIO_Port;
      break;

    case BSP_LED4:
      gpio_pin = LED4_Pin;
      gpiox = LED4_GPIO_Port;
      break;

    case BSP_LED5:
      gpio_pin = LED5_Pin;
      gpiox = LED5_GPIO_Port;
      break;

    case BSP_LED6:
      gpio_pin = LED6_Pin;
      gpiox = LED6_GPIO_Port;
      break;

    case BSP_LED7:
      gpio_pin = LED7_Pin;
      gpiox = LED7_GPIO_Port;
      break;

    case BSP_LED8:
      gpio_pin = LED8_Pin;
      gpiox = LED8_GPIO_Port;
      break;

    case BSP_LED_RED:
      gpiox = LED_R_GPIO_Port;
      gpio_pin = LED_R_Pin;
      break;

    case BSP_LED_GRN:
      gpiox = LED_G_GPIO_Port;
      gpio_pin = LED_G_Pin;
      break;
  }

  switch (s) {
    case BSP_LED_ON:
      HAL_GPIO_WritePin(gpiox, gpio_pin, GPIO_PIN_RESET);
      break;

    case BSP_LED_OFF:
      HAL_GPIO_WritePin(gpiox, gpio_pin, GPIO_PIN_SET);
      break;

    case BSP_LED_TAGGLE:
      HAL_GPIO_TogglePin(gpiox, gpio_pin);
      break;
  }

  return 0;
}
