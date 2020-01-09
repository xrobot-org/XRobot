#pragma once

#include "board.h"

typedef enum {
	JOYSTICK_UP,
	JOYSTICK_DOWN,
	JOYSTICK_LEFT,
	JOYSTICK_RIGHT,
	JOYSTICK_PRESSED,
	JOYSTICK_MID,
} Joystick_Status_t;


Board_Status_t Joystick_Update(Joystick_Status_t* val);
Board_Status_t Joystick_WaitInput(void);
Board_Status_t Joystick_WaitNoInput(void);
