#ifndef __BSP_IO__H
#define __BSP_IO__H

#include "bsp_common.h"

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
} LED_NumTypedef;

typedef enum {
	LED_ON,
	LED_OFF,
	LED_TAGGLE,
} LED_StatusTypedef;

typedef enum {
	JOYSTICK_UP,
	JOYSTICK_DOWN,
	JOYSTICK_LEFT,
	JOYSTICK_RIGHT,
	JOYSTICK_PRESSED,
	JOYSTICK_MID,
} Joystick_StatusTypedef;

typedef enum {
	PWM_A,
	PWM_B,
	PWM_C,
	PWM_D,
	PWM_E,
	PWM_F,
	PWM_G,
	PWM_H,
	PWM_S,
	PWM_T,
	PWM_U,
	PWM_V,
	PWM_W,
	PWM_X,
	PWM_Y,
	PWM_Z,
	PWM_IMU_HEAT,
} PWM_NumTypedef;

typedef enum {
	POWER_OFF,
	POWER_ON,
} Power_StatusTypedef;

typedef enum {
	POWER_PORT1,
	POWER_PORT2,
	POWER_PORT3,
	POWER_PORT4,
} Power_PortTypedef;

BSP_StatusTypedef LED_Set(LED_NumTypedef n, LED_StatusTypedef s);

BSP_StatusTypedef Joystick_Update(Joystick_StatusTypedef* val);
BSP_StatusTypedef Joystick_WaitInput(void);
BSP_StatusTypedef Joystick_WaitNoInput(void);

BSP_StatusTypedef PWM_Start(PWM_NumTypedef n);
BSP_StatusTypedef PWM_Set(PWM_NumTypedef n, float duty_cycle);

BSP_StatusTypedef Power_Set(Power_PortTypedef port ,Power_StatusTypedef s);

#endif
