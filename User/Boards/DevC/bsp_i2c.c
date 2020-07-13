/* Includes ------------------------------------------------------------------*/
#include "bsp_i2c.h"

#include "main.h"
#include "i2c.h"

/* Private define ------------------------------------------------------------*/
#define COMP_I2C I2C2
/* #define XXX_I2C I2CX */

#define COMP_I2C_HANDLE (hi2c2)
/* #define XXX_I2C_HANDLE (hi2cx) */

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct {
	struct {
		void (*MasterTxCpltCallback)(void);	/* I2C Master Tx Transfer completed callback */
		void (*MasterRxCpltCallback)(void);	/* I2C Master Rx Transfer completed callback */
		void (*SlaveTxCpltCallback)(void);	/* I2C Slave Tx Transfer completed callback  */
		void (*SlaveRxCpltCallback)(void);	/* I2C Slave Rx Transfer completed callback  */
		void (*ListenCpltCallback)(void);	/* I2C Listen Complete callback */
		void (*MemTxCpltCallback)(void);	/* I2C Memory Tx Transfer completed callback */
		void (*MemRxCpltCallback)(void);	/* I2C Memory Rx Transfer completed callback */
		void (*ErrorCallback)(void);		/* I2C Error callback */
		void (*AbortCpltCallback)(void);	/* I2C Abort callback */
	} comp;
	
	/*
	struct {
		void (*MasterTxCpltCallback)(void);
		void (* MasterRxCpltCallback)(void);
		void (* SlaveTxCpltCallback)(void);
		void (* SlaveRxCpltCallback)(void);
		void (* ListenCpltCallback)(void);
		void (* MemTxCpltCallback)(void);
		void (* MemRxCpltCallback)(void);
		void (* ErrorCallback)(void);
		void (* AbortCpltCallback)(void);
		void (* AddrCallback)(uint8_t TransferDirection, uint16_t AddrMatchCode);
	} xxx;
	*/
} static bsp_i2c_callback;

/* Private function  ---------------------------------------------------------*/
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == COMP_I2C) {
		if (bsp_i2c_callback.comp.MasterTxCpltCallback)
			bsp_i2c_callback.comp.MasterTxCpltCallback();
	}
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == COMP_I2C) {
		if (bsp_i2c_callback.comp.MasterRxCpltCallback)
			bsp_i2c_callback.comp.MasterRxCpltCallback();
	}
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == COMP_I2C) {
		if (bsp_i2c_callback.comp.SlaveTxCpltCallback)
			bsp_i2c_callback.comp.SlaveTxCpltCallback();
	}
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == COMP_I2C) {
		if (bsp_i2c_callback.comp.SlaveRxCpltCallback)
			bsp_i2c_callback.comp.SlaveRxCpltCallback();
	}
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == COMP_I2C) {
		if (bsp_i2c_callback.comp.ListenCpltCallback)
			bsp_i2c_callback.comp.ListenCpltCallback();
	}
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == COMP_I2C) {
		if (bsp_i2c_callback.comp.MemTxCpltCallback)
			bsp_i2c_callback.comp.MemTxCpltCallback();
	}
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == COMP_I2C) {
		if (bsp_i2c_callback.comp.MemRxCpltCallback)
			bsp_i2c_callback.comp.MemRxCpltCallback();
	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == COMP_I2C) {
		if (bsp_i2c_callback.comp.ErrorCallback)
			bsp_i2c_callback.comp.ErrorCallback();
	}
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == COMP_I2C) {
		if (bsp_i2c_callback.comp.AbortCpltCallback)
			bsp_i2c_callback.comp.AbortCpltCallback();
	}
}

/* Exported functions --------------------------------------------------------*/
int8_t BSP_I2C_RegisterCallback(BSP_I2C_t i2c, BSP_I2C_Callback_t type, void (*callback)(void)) {
	if (callback == NULL)
		return -1;
	
	switch (i2c) {
		case BSP_I2C_COMP:
			switch (type) {
				case HAL_I2C_MASTER_TX_COMPLETE_CB:
					bsp_i2c_callback.comp.MasterTxCpltCallback = callback;
					break;
				case HAL_I2C_MASTER_RX_COMPLETE_CB:
					bsp_i2c_callback.comp.MasterRxCpltCallback = callback;
					break;
				case HAL_I2C_SLAVE_TX_COMPLETE_CB:
					bsp_i2c_callback.comp.SlaveTxCpltCallback = callback;
					break;
				case HAL_I2C_SLAVE_RX_COMPLETE_CB:
					bsp_i2c_callback.comp.SlaveRxCpltCallback = callback;
					break;
				case HAL_I2C_LISTEN_COMPLETE_CB:
					bsp_i2c_callback.comp.ListenCpltCallback = callback;
					break;
				case HAL_I2C_MEM_TX_COMPLETE_CB:
					bsp_i2c_callback.comp.MemTxCpltCallback = callback;
					break;
				case HAL_I2C_MEM_RX_COMPLETE_CB:
					bsp_i2c_callback.comp.MemRxCpltCallback = callback;
					break;
				case HAL_I2C_ERROR_CB:
					bsp_i2c_callback.comp.ErrorCallback = callback;
					break;
				case HAL_I2C_ABORT_CB:
					bsp_i2c_callback.comp.AbortCpltCallback = callback;
					break;
				default:
					return -1;
			}
			break;
		/*	
		case BSP_I2C_XXX:
			switch (type) {
				case HAL_I2C_MASTER_TX_COMPLETE_CB:
					bsp_i2c_callback.comp.TxCpltCallback = callback;
					break;
				case HAL_I2C_MASTER_RX_COMPLETE_CB:
					bsp_i2c_callback.comp.RxCpltCallback = callback;
					break;
				case HAL_I2C_SLAVE_TX_COMPLETE_CB:
					bsp_i2c_callback.comp.TxRxCpltCallback = callback;
					break;
				case HAL_I2C_SLAVE_RX_COMPLETE_CB:
					bsp_i2c_callback.comp.TxHalfCpltCallback = callback;
					break;
				case HAL_I2C_LISTEN_COMPLETE_CB:
					bsp_i2c_callback.comp.RxHalfCpltCallback = callback;
					break;
				case HAL_I2C_MEM_TX_COMPLETE_CB:
					bsp_i2c_callback.comp.TxRxHalfCpltCallback = callback;
					break;
				case HAL_I2C_MEM_RX_COMPLETE_CB:
					bsp_i2c_callback.comp.ErrorCallback = callback;
					break;
				case HAL_I2C_ERROR_CB:
					bsp_i2c_callback.comp.AbortCpltCallback = callback;
					break;
				case HAL_I2C_ABORT_CB:
					bsp_i2c_callback.comp.AbortCpltCallback = callback;
					break;
				default:
					return -1;
			}
			break;
		*/
		default:
			return -1;
	}
	return 0;
}

int8_t BSP_I2C_Transmit(BSP_I2C_t i2c, uint16_t address, uint8_t *data, uint16_t len, uint32_t time_out) {
	if (data == NULL)
		return -1;
	
	switch (i2c) {
		case BSP_I2C_COMP:
			return HAL_I2C_Master_Transmit(&COMP_I2C_HANDLE, address, data, len, time_out);
		
		/*
		case BSP_I2C_XXX:
			HAL_I2C_Master_Transmit(&XXX_I2C_HANDLE, address, data, len, time_out);
		*/
		default:
			return -1;
	}
}

int8_t BSP_I2C_TransmitDMA(BSP_I2C_t i2c, uint16_t address, uint8_t *data, uint16_t len) {
	if (data == NULL)
		return -1;
	
	switch (i2c) {
		case BSP_I2C_COMP:
			return HAL_I2C_Master_Transmit_DMA(&COMP_I2C_HANDLE, address, data, len);
		
		default:
			return -1;
	}
}

int8_t BSP_I2C_Receive(BSP_I2C_t i2c, uint16_t address, uint8_t *data, uint16_t len, uint32_t time_out) {
	if (data == NULL)
		return -1;

	switch (i2c) {
		case BSP_I2C_COMP:
			return HAL_I2C_Master_Receive(&COMP_I2C_HANDLE, address, data, len, time_out);
		
		default:
			return -1;
	}
}

int8_t BSP_I2C_ReceiveDMA(BSP_I2C_t i2c, uint16_t address, uint8_t *data, uint16_t len) {
	if (data == NULL)
		return -1;

	switch (i2c) {
		case BSP_I2C_COMP:
			return HAL_I2C_Master_Receive_DMA(&COMP_I2C_HANDLE, address, data, len);
		
		default:
			return -1;
	}
}

