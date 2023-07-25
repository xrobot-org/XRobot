#include "bsp.h"

#include "bsp_can.h"
#include "bsp_uart.h"
#include "main.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_it.h"

extern TIM_HandleTypeDef htim6;

void bsp_init() {
  uwTickPrio = 0;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  bsp_uart_init();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_ADC3_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM4_Init();
  MX_TIM5_Init();
  MX_USART3_UART_Init();
  MX_TIM8_Init();
  MX_RNG_Init();
  MX_I2C2_Init();
  MX_I2C3_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM10_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  MX_TIM7_Init();
  MX_USB_OTG_FS_PCD_Init();
#if !MCU_DEBUG_BUILD
  MX_IWDG_Init();
#endif
  HAL_TIM_Base_Stop_IT(&htim6);
}
