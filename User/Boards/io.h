#pragma once

#include "board.h"

typedef enum {
	LED_ON,
	LED_OFF,
	LED_TAGGLE,
} LED_Status_t;

#ifdef STM32F407xx

typedef enum {
	LED_RED,
	LED_GRN,
	LED_BLU,
} LED_Num_t;

#elif defined STM32F427xx

typedef enum {
	LED1,
	LED2,
	LED3,
	LED4,
	LED5,
	LED6,
	LED7,
	LED8,
	LED_RED,
	LED_GRN,
} LED_Num_t;

typedef enum {
	POWER_PORT1,
	POWER_PORT2,
	POWER_PORT3,
	POWER_PORT4,
} Power_Port_t;

#endif

int LED_Set(LED_Num_t n, LED_Status_t s);


int Power_On(Power_Port_t port);
int Power_Off(Power_Port_t port);

int Laser_On(void);
int Laser_Off(void);

int Buzzer_On(uint16_t pulse);
int Buzzer_Off(void);
