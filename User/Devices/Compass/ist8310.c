#pragma once

#include "main.h"

#define IST8310_WAI 							(0x00)
#define IST8310_STAT1 							(0x02)
#define IST8310_DATAXL 							(0x03)
#define IST8310_DATAXM 							(0x04)
#define IST8310_DATAYL 							(0x05)
#define IST8310_DATAYM 							(0x06)
#define IST8310_DATAZL 							(0x07)
#define IST8310_DATAZM 							(0x08)
#define IST8310_STAT2 							(0x08)
#define IST8310_CNTL1		 					(0x0A)
#define IST8310_CNTL2		 					(0x0B)
#define IST8310_STR 							(0x0C)
#define IST8310_TEMPL 							(0x1C)
#define IST8310_TEMPH 							(0x1D)
#define IST8310_AVGCNTL 						(0x41)
#define IST8310_PDCNTL 							(0x42)

#define IST8310_ADDRESS 						(0x0E)
#define IST8310_ID		 						(0x10)

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


void DR16_Init(DR16_t* pdr, const uint8_t* raw); /*Need nodifed.*/
void DR16_Decode(DR16_t* pdr, const uint8_t* raw);
void DR16_Restart(DR16_t* pdr, const uint8_t* raw); /*Need nodifed.*/
void DR16_HandleError(DR16_t* pdr, const uint8_t* raw); /*Need nodifed.*/

