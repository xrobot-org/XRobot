#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	Servo_A,
	Servo_B,
	Servo_C,
	Servo_D,
	Servo_E,
	Servo_F,
	Servo_G,
	Servo_H,
	Servo_S,
	Servo_T,
	Servo_U,
	Servo_V,
	Servo_W,
	Servo_X,
	Servo_Y,
	Servo_Z,
} BSP_Servo_Channel_t;

/* Exported functions prototypes ---------------------------------------------*/
int BSP_Servo_Start(BSP_Servo_Channel_t ch);
int BSP_Servo_Set(BSP_Servo_Channel_t ch, uint8_t angle);
int BSP_Servo_Stop(BSP_Servo_Channel_t ch);
