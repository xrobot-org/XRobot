#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Include cmsis_os.h头文件 */
#include "cmsis_os.h"

#include "ahrs.h"

/* Exported constants --------------------------------------------------------*/
#define IST8310_OK			(0)
#define IST8310_ERR		(-1)
#define IST8310_ERR_NULL	(-2)
#define IST8310_ERR_INITED	(-3)
#define IST8310_ERR_NO_DEV	(-4)

#define IST8310_SIGNAL_MAGN_NEW_DATA	(1u<<10)
#define IST8310_SIGNAL_MAGN_RAW_REDY	(1u<<11)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId_t received_alert;

	uint8_t raw[6];
	
	AHRS_Magn_t magn;
	
	struct {
		int magn_offset[3];
		int magn_scale[3];
	} cali;
} IST8310_t;

/* Exported functions prototypes ---------------------------------------------*/
int IST8310_Init(IST8310_t *ist8310);
IST8310_t *IST8310_GetDevice(void);

int IST8310_Restart(void);

int IST8310_Receive(IST8310_t *ist8310);
int IST8310_Parse(IST8310_t *ist8310);
