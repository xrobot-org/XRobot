#include "bsp_wdg.h"

#include "main.h"

extern IWDG_HandleTypeDef hiwdg;

void bsp_wdg_refresh(void) { HAL_IWDG_Refresh(&hiwdg); }
