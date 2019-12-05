#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef enum {
	BOARD_FAIL = -1,
	BOARD_OK,
} Board_Status_t;

void Board_Delay(uint32_t ms);
