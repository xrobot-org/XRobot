#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "component\user_math.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	BSP_LED_ON,
	BSP_LED_OFF,
	BSP_LED_TAGGLE,
} BSP_LED_Status_t;

typedef enum {
#ifdef DEV_BOARD_A
	BSP_LED1,
	BSP_LED2,
	BSP_LED3,
	BSP_LED4,
	BSP_LED5,
	BSP_LED6,
	BSP_LED7,
	BSP_LED8,
#elif defined DEV_BOARD_C
	BSP_LED_BLU,
#endif
	BSP_LED_RED,
	BSP_LED_GRN,
} BSP_LED_Channel_t;

/* Exported functions prototypes ---------------------------------------------*/

/* 用于A板时，会无视duty_cycle的值。使用B板时，duty_cycle才有效*/
int8_t BSP_LED_Set(BSP_LED_Channel_t ch, BSP_LED_Status_t s, float duty_cycle);
