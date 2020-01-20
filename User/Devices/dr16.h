#pragma once

/* Includes ------------------------------------------------------------------*/
/* Include cmsis_os2.h头文件。*/
#include "cmsis_os2.h"

/* Include board.h头文件。*/
#include "board.h"

/* Exported constants --------------------------------------------------------*/
#define DR16_SIGNAL_DATA_RECV (1u<<0)

#define RC_CH_VALUE_MIN         ((uint16_t)364)
#define RC_CH_VALUE_OFFSET      ((uint16_t)1024)
#define RC_CH_VALUE_MAX         ((uint16_t)1684)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId_t received_alert;
	
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

/* Exported functions prototypes ---------------------------------------------*/
int DR16_Init(DR16_t* pdr); 
int DR16_Parse(DR16_t* pdr, const uint8_t* raw);
int DR16_Restart(void);



