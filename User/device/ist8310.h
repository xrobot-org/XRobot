#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include <cmsis_os2.h>

#include "component\ahrs.h"
#include "device\device.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId_t thread_alert;

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

#ifdef __cplusplus
}
#endif
