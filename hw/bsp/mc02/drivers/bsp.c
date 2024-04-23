#include "bsp.h"

#include "bsp_uart.h"
#include "main.h"
#include "stm32h7xx_hal_tim.h"
#include "stm32h7xx_it.h"

extern TIM_HandleTypeDef htim23;
void bsp_init(void) {
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  bsp_uart_init();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_BDMA_Init();
  MX_TIM12_Init();
  MX_SPI2_Init();
  MX_TIM3_Init();
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();
  MX_FDCAN3_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_OCTOSPI2_Init();
  MX_USB_OTG_HS_PCD_Init();
  MX_TIM7_Init();
  MX_SPI6_Init();
  MX_ADC1_Init();
  MX_USART3_UART_Init();
  MX_UART7_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART10_UART_Init();
  MX_UART5_Init();
  HAL_TIM_Base_Stop_IT(&htim23);
}
