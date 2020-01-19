#pragma once

#include "board.h"

typedef enum {
    OLED_PEN_CLEAR = 0,
    OLED_PEN_WRITE = 1,
    OLED_PEN_INVERSION = 2,
} OLED_PenTypedef;

int OLED_DisplayOn(void);
int OLED_DisplayOff(void);
int OLED_Refresh(void);
int OLED_SetAll(OLED_PenTypedef pen);
int OLED_Init(void);
int OLED_Print(const char* str);
int OLED_Rewind(void);
