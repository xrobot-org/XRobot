#pragma once

/* Includes ----------------------------------------------------------------- */
#include <stdbool.h>
#include <stdint.h>

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
typedef enum {
  OLED_PEN_CLEAR = 0,
  OLED_PEN_WRITE = 1,
  OLED_PEN_INVERSION = 2,
} OLED_Pen_t;

typedef struct {
  uint8_t column;
  s uint8_t page;
} OLED_Cursor_t;

typedef struct {
  OLED_Cursor_t cursor;
  uint8_t gram[8][128];
  bool modified;
} OLED_t;

/* Exported functions prototypes -------------------------------------------- */
uint8_t OLED_Init(OLED_t *oled);
OLED_t *OLED_GetDevice(void);

uint8_t OLED_PrintRam(OLED_t *oled, const char *str);
uint8_t OLED_RewindRam(OLED_t *oled);
uint8_t OLED_SetAllRam(OLED_t *oled, OLED_Pen_t pen);

uint8_t OLED_DisplayOn(OLED_t *oled);
uint8_t OLED_DisplayOff(OLED_t *oled);
uint8_t OLED_Refresh(OLED_t *oled);
