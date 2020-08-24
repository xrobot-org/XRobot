#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <i2c.h>

#include "bsp/bsp.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	BSP_I2C_COMP,
	/* BSP_I2C_XXX,*/
	BSP_I2C_NUM,
	BSP_I2C_ERR,
} BSP_I2C_t;

typedef enum {
	HAL_I2C_MASTER_TX_CPLT_CB,
	HAL_I2C_MASTER_RX_CPLT_CB,
	HAL_I2C_SLAVE_TX_CPLT_CB,
	HAL_I2C_SLAVE_RX_CPLT_CB,
	HAL_I2C_LISTEN_CPLT_CB,
	HAL_I2C_MEM_TX_CPLT_CB,
	HAL_I2C_MEM_RX_CPLT_CB,
	HAL_I2C_ERROR_CB,
	HAL_I2C_ABORT_CPLT_CB,
	BSP_I2C_CB_NUM,
} BSP_I2C_Callback_t;

/* Exported functions prototypes ---------------------------------------------*/
I2C_HandleTypeDef *BSP_I2C_GetHandle(BSP_I2C_t i2c);
int8_t BSP_I2C_RegisterCallback(BSP_I2C_t i2c, BSP_I2C_Callback_t type, void (*callback)(void));

#ifdef __cplusplus
}
#endif
