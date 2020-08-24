#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	POWER_PORT1,
	POWER_PORT2,
	POWER_PORT3,
	POWER_PORT4,
} BSP_Power_Port_t;

/* Exported functions prototypes ---------------------------------------------*/
int BSP_Power_Set(BSP_Power_Port_t port, bool s);

#ifdef __cplusplus
}
#endif
