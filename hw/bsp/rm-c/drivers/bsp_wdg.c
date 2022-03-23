#include "bsp_wdg.h"

void bsp_wdg_refresh(void) { HAL_IWDG_Refresh(&hiwdg); }
