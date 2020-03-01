/* Includes ------------------------------------------------------------------*/
#include "bsp_rand.h"

#include "rng.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
uint32_t BSP_GetRandomNum(void) {
	return HAL_RNG_GetRandomNumber(&hrng);
}

int BSP_GetRandomRangle(int min, int max) {
	return 0;
}
