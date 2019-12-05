#ifndef __BSP_OLED__H
#define __BSP_OLED__H

#include "board.h"

typedef enum {
    OLED_PEN_CLEAR = 0,
    OLED_PEN_WRITE = 1,
    OLED_PEN_INVERSION = 2,
} OLED_PenTypedef;

Board_Status_t OLED_DisplayOn(void);
Board_Status_t OLED_DisplayOff(void);
Board_Status_t OLED_Refresh(void);
Board_Status_t OLED_SetAll(OLED_PenTypedef pen);
Board_Status_t OLED_Init(void);
Board_Status_t OLED_Print(const char* str);
Board_Status_t OLED_Rewind(void);

#endif
