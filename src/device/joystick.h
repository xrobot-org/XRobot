#pragma once

/* Includes ----------------------------------------------------------------- */
/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
typedef enum {
  JOYSTICK_UP,
  JOYSTICK_DOWN,
  JOYSTICK_LEFT,
  JOYSTICK_RIGHT,
  JOYSTICK_PRESSED,
  JOYSTICK_MID,
} Joystick_Status_t;

/* Exported functions prototypes -------------------------------------------- */
uint8_t Joystick_Update(Joystick_Status_t *val);
uint8_t Joystick_WaitInput(void);
uint8_t Joystick_WaitNoInput(void);
