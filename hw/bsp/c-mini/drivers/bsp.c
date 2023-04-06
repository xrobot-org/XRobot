#include "bsp.h"

#include "bsp_can.h"
#include "bsp_uart.h"
#include "main.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_it.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim14;

void bsp_init() {
  // uwTickPrio = TICK_INT_PRIORITY;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_CAN1_Init();
  MX_UART4_Init();
  MX_CAN2_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  MX_UART5_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_USART6_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_TIM14_Init();
  MX_TIM7_Init();
  HAL_TIM_Base_Start(&htim14);
#if !MCU_DEBUG_BUILD
  MX_IWDG_Init();
#endif
  HAL_TIM_Base_Stop_IT(&htim1);

  bsp_uart_init();
}
