#include "bsp_mm.h"

#include "FreeRTOS.h"

inline void *BSP_Malloc(size_t size) { return pvPortMalloc(size); }

inline void BSP_Free(void *pv) {
  vPortFree(pv);
  pv = NULL;
}
