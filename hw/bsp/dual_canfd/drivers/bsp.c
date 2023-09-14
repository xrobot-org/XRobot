#include "bsp.h"

#include "bsp_can.h"
#include "bsp_uart.h"
#include "main.h"
#include "stm32g0xx_hal_msp.c"
#include "stm32g0xx_it.c"

void bsp_init() {
  SCB->VTOR = FLASH_BASE;

  /* Reset of all peripherals, Initializes the Flash interface and the
   * Systick.
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_LPUART1_UART_Init();
  MX_FDCAN1_Init();
  MX_FDCAN2_Init();

  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(SysTick_IRQn);
}
