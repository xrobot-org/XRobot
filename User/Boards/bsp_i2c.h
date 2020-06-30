#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	BSP_I2C_COMP,
	/* BSP_I2C_XXX,*/
} BSP_I2C_t;

typedef enum {
	HAL_I2C_MASTER_TX_COMPLETE_CB,	/* I2C Master Tx Transfer completed callback ID  */
	HAL_I2C_MASTER_RX_COMPLETE_CB,	/* I2C Master Rx Transfer completed callback ID  */
	HAL_I2C_SLAVE_TX_COMPLETE_CB,	/* I2C Slave Tx Transfer completed callback ID   */
	HAL_I2C_SLAVE_RX_COMPLETE_CB,	/* I2C Slave Rx Transfer completed callback ID   */
	HAL_I2C_LISTEN_COMPLETE_CB,		/* I2C Listen Complete callback ID               */
	HAL_I2C_MEM_TX_COMPLETE_CB,		/* I2C Memory Tx Transfer callback ID            */
	HAL_I2C_MEM_RX_COMPLETE_CB,		/* I2C Memory Rx Transfer completed callback ID  */
	HAL_I2C_ERROR_CB,				/* I2C Error callback ID                         */
	HAL_I2C_ABORT_CB,				/* I2C Abort callback ID                         */
} BSP_I2C_Callback_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t BSP_I2C_RegisterCallback(BSP_I2C_t i2c, BSP_I2C_Callback_t type, void (*callback)(void));

int8_t BSP_I2C_Transmit(BSP_I2C_t i2c, uint16_t address, uint8_t *data, uint16_t len, uint32_t time_out);
int8_t BSP_I2C_TransmitDMA(BSP_I2C_t i2c, uint16_t address, uint8_t *data, uint16_t len);

int8_t BSP_I2C_Receive(BSP_I2C_t i2c, uint16_t address, uint8_t *data, uint16_t len, uint32_t time_out);
int8_t BSP_I2C_ReceiveDMA(BSP_I2C_t i2c, uint16_t address, uint8_t *data, uint16_t len);
