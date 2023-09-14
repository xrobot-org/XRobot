#include "bsp.h"

#include "bsp_can.h"
#include "bsp_uart.h"
#include "main.h"
#include "stm32g4xx_hal_i2c.h"
#include "stm32g4xx_hal_msp.c"
#include "stm32g4xx_hal_tim.h"
#include "stm32g4xx_it.c"

void bsp_init() {
  uwTickPrio = TICK_INT_PRIORITY;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FDCAN1_Init();
  MX_USART2_UART_Init();
}
