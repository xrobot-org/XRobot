#pragma once

#include "main.h"

typedef struct {
	uint8_t raw[18];
	
	struct {
		int32_t ch[5];
		int32_t sw[2];
	} rc;
	
	struct {
		int32_t x;
		int32_t y;
		int32_t z;
		int32_t press_left;
		int32_t press_right;
	} mouse;
	
	int32_t key;
} DR16_t;

void DR16_Decode(DR16_t* raw);
