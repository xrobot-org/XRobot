#include "bsp.h"

#include "bsp_can.h"
#include "main.h"
#include "stm32f1xx_it.h"

void bsp_init() {
  uwTickPrio = TICK_INT_PRIORITY;
  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  HAL_InterruptInit();

  /* Initialize all configured peripherals */
  MX_DMA_Init();
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_CAN_Init();
  // MX_I2C1_Init();
  // MX_USART1_UART_Init();
  MX_TIM2_Init();
}
