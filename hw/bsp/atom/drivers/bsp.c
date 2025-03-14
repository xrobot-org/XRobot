#include "bsp.h"

#include "bsp_can.h"
#include "bsp_sys.h"
#include "bsp_uart.h"
#include "main.h"
#include "stm32g4xx_hal_i2c.h"
#include "stm32g4xx_hal_msp.c"
#include "stm32g4xx_hal_tim.h"
#include "stm32g4xx_it.c"

extern TIM_HandleTypeDef htim15;

void bsp_init() {
  uwTickPrio = TICK_INT_PRIORITY;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_FDCAN1_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_SPI3_Init();
  MX_TIM1_Init();

  bsp_uart_init();
}
