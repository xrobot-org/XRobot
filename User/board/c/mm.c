/* Includes ------------------------------------------------------------------*/
#include "board\mm.h"

#include "FreeRTOS.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
inline void *BSP_Malloc(size_t size) {
	return pvPortMalloc(size);
}

inline void BSP_Free(void *pv) {
	vPortFree(pv);
}
