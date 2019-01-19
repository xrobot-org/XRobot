#ifndef __BSP_OLED__H
#define __BSP_OLED__H

#include "bsp_common.h"

typedef enum {
    OLED_PEN_CLEAR = 0,
    OLED_PEN_WRITE = 1,
    OLED_PEN_INVERSION = 2,
} OLED_PenTypedef;

BSP_StatusTypedef OLED_DisplayOn(void);
BSP_StatusTypedef OLED_DisplayOff(void);
BSP_StatusTypedef OLED_Refresh(void);
BSP_StatusTypedef OLED_SetAll(OLED_PenTypedef pen);
BSP_StatusTypedef OLED_Init(void);
BSP_StatusTypedef OLED_Print(const char *str);
BSP_StatusTypedef OLED_Rewind(void);

#endif
