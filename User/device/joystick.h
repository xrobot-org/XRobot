#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	JOYSTICK_UP,
	JOYSTICK_DOWN,
	JOYSTICK_LEFT,
	JOYSTICK_RIGHT,
	JOYSTICK_PRESSED,
	JOYSTICK_MID,
} Joystick_Status_t;

/* Exported functions prototypes ---------------------------------------------*/
int Joystick_Update(Joystick_Status_t *val);
int Joystick_WaitInput(void);
int Joystick_WaitNoInput(void);

#ifdef __cplusplus
}
#endif
