#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <spi.h>

#include "bsp/bsp.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	BSP_SPI_OLED,
	BSP_SPI_IMU,
	/* BSP_SPI_XXX,*/
	BSP_SPI_NUM,
	BSP_SPI_ERR,
} BSP_SPI_t;

typedef enum {
	BSP_SPI_TX_CPLT_CB,
	BSP_SPI_RX_CPLT_CB,
	BSP_SPI_TX_RX_CPLT_CB,
	BSP_SPI_TX_HALF_CPLT_CB,
	BSP_SPI_RX_HALF_CPLT_CB,
	BSP_SPI_TX_RX_HALF_CPLT_CB,
	BSP_SPI_ERROR_CB,
	BSP_SPI_ABORT_CPLT_CB,
	BSP_SPI_CB_NUM,
} BSP_SPI_Callback_t;

/* Exported functions prototypes ---------------------------------------------*/
SPI_HandleTypeDef *BSP_SPI_GetHandle(BSP_SPI_t spi);
int8_t BSP_SPI_RegisterCallback(BSP_SPI_t spi, BSP_SPI_Callback_t type, void (*callback)(void));

#ifdef __cplusplus
}
#endif
