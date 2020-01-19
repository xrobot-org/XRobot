#pragma once

#include "board.h"

#define DR16_RX_BUF_SIZE 36u

#define RC_CH_VALUE_MIN         ((uint16_t)364)
#define RC_CH_VALUE_OFFSET      ((uint16_t)1024)
#define RC_CH_VALUE_MAX         ((uint16_t)1684)

typedef struct {
	
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


int DR16_Init(DR16_t* pdr); 
int DR16_Parse(DR16_t* pdr, const uint8_t* raw);
int DR16_Restart(void);

