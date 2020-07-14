#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Include cmsis_os2.h头文件 */
#include "cmsis_os2.h"

#include "component\ahrs.h"

/* Exported constants --------------------------------------------------------*/
#define IST8310_OK			(0)
#define IST8310_ERR		(-1)
#define IST8310_ERR_NULL	(-2)
#define IST8310_ERR_INITED	(-3)
#define IST8310_ERR_NO_DEV	(-4)

#define IST8310_SIGNAL_MAGN_NEW_DATA	(1u<<9)
#define IST8310_SIGNAL_MAGN_RAW_REDY	(1u<<10)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId_t received_alert;

	uint8_t raw[6];
	
	AHRS_Magn_t magn;
	
	struct {
		int8_t magn_offset[3];
		int8_t magn_scale[3];
	} cali;
} IST8310_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t IST8310_Init(IST8310_t *ist8310, osThreadId_t thread_alert);
IST8310_t *IST8310_GetDevice(void);

int8_t IST8310_Restart(void);

int8_t IST8310_Receive(IST8310_t *ist8310);
int8_t IST8310_Parse(IST8310_t *ist8310);
