#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

/* Include cmsis_os.h头文件 */
#include "cmsis_os.h"

/* Exported constants --------------------------------------------------------*/
#define DR16_OK			(0)
#define DR16_ERR_NULL	(-1)

#define DR16_SIGNAL_RAW_REDY	(1u<<5)

#define DR16_CH_VALUE_MIN			(364u)
#define DR16_CH_VALUE_MID			(1024u)
#define DR16_CH_VALUE_MAX			(1684u)

#define DR16_SW_UP			(1u)
#define DR16_SW_MID			(3u)
#define DR16_SW_DOWN		(2u)

#define DR16_KEY_MASK_W		(1u<<0)
#define DR16_KEY_MASK_S		(1u<<1)
#define DR16_KEY_MASK_A		(1u<<2)
#define DR16_KEY_MASK_D		(1u<<3)
#define DR16_KEY_MASK_Q		(1u<<4)
#define DR16_KEY_MASK_E		(1u<<5)
#define DR16_KEY_MASK_SHIST	(1u<<6)
#define DR16_KEY_MASK_CTRL	(1u<<7)

#define DR16_RX_BUF_NUM 36u

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId received_alert;

	uint8_t raw[DR16_RX_BUF_NUM];
	
	struct {
		struct {
			uint16_t ch[5];
			uint8_t sw[2];
		} rc;
		
		struct {
			int16_t x;
			int16_t y;
			int16_t z;
			bool press_left;
			bool press_right;
		} mouse;
		
		uint16_t key;
		uint16_t re; 
	} data;
} DR16_t;

/* Exported functions prototypes ---------------------------------------------*/
int DR16_Init(DR16_t *dr16);
DR16_t *DR16_GetDevice(void);

int DR16_StartReceiving(DR16_t *dr16);

int DR16_Parse(DR16_t *dr16);
int DR16_Restart(void);



