#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Include cmsis_os.h头文件 */
#include "cmsis_os.h"

#include "ahrs.h"

/* Exported constants --------------------------------------------------------*/
#define COMP_OK			(0)
#define COMP_ERR		(-1)
#define COMP_ERR_NULL	(-2)
#define COMP_ERR_INITED	(-3)
#define COMP_ERR_NO_DEV	(-4)

#define COMP_SIGNAL_RAW_MAGN_REDY	(1u<<9)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId received_alert;

	uint8_t raw[6];
	
	AHRS_Magn_t magn;
	
	struct {
		int magn_offset[3];
		int magn_scale[3];
	} cali;
} COMP_t;

/* Exported functions prototypes ---------------------------------------------*/
