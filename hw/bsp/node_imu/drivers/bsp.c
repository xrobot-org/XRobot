#include "bsp.h"

#include "bsp_can.h"
#include "main.h"
#include "stm32f3xx_hal_msp.c"
#include "stm32f3xx_hal_tim.h"
#include "stm32f3xx_it.c"

extern TIM_HandleTypeDef htim1;

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
  MX_CAN_Init();
  MX_SPI1_Init();
  MX_USB_PCD_Init();
  HAL_TIM_Base_Stop_IT(&htim1);
}
