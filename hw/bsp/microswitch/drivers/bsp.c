#include "bsp.h"

#include "bsp_can.h"
#include "bsp_uart.h"
#include "main.h"
#include "stm32f1xx_it.h"

void bsp_init() {
  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_CAN_Init();
  MX_USART1_UART_Init();

  bsp_uart_init();
}
