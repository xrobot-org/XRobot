/* Includes ----------------------------------------------------------------- */
#include "bsp_delay.h"
#include "cmsis_os.h"
#include "hal.h"

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
uint8_t BSP_Delay(uint32_t ms) {
  if (osKernelRunning()) {
    osDelay(ms);
  } else {
    HAL_Delay(ms);
  }
  return 0;
}