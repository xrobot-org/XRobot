#pragma once


/* Includes ------------------------------------------------------------------*/
/* Include cmsis_os2.h头文件 */
#include "cmsis_os2.h"

#include "cmd.h"
#include "user_math.h"

/* Exported constants --------------------------------------------------------*/
#define DR16_OK			(0)
#define DR16_ERR_NULL	(-1)

#define DR16_SIGNAL_RAW_REDY	(1u<<7)

#define DR16_CH_VALUE_MIN		(364u)
#define DR16_CH_VALUE_MID		(1024u)
#define DR16_CH_VALUE_MAX		(1684u)

#define DR16_RX_BUF_LENGTH 36u

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/


typedef struct {
	osThreadId_t thread_alert;

	uint8_t raw[DR16_RX_BUF_LENGTH];
} DR16_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t DR16_Init(DR16_t *dr16, osThreadId_t thread_alert);
DR16_t *DR16_GetDevice(void);
int8_t DR16_Restart(void);

int8_t DR16_StartReceiving(DR16_t *dr16);
int8_t DR16_Parse(const DR16_t *dr16, CMD_RC_t *rc);
