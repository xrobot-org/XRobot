/* Includes ------------------------------------------------------------------*/
#include "bsp\i2c.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct {
	struct {
		void (*MasterTxCpltCallback)(void);
		void (*MasterRxCpltCallback)(void);
		void (*SlaveTxCpltCallback)(void);
		void (*SlaveRxCpltCallback)(void);
		void (*ListenCpltCallback)(void);
		void (*MemTxCpltCallback)(void);
		void (*MemRxCpltCallback)(void);
		void (*ErrorCallback)(void);
		void (*AbortCpltCallback)(void);
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
static I2C_TypeDef *I2C_GetInstance(BSP_I2C_t i2c) {
	switch (i2c) {
		case BSP_I2C_COMP:
			return I2C2;
		/*
		case BSP_I2C_XXX:
			return &I2CX;
		*/
		default:
			return NULL;
	}
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == I2C_GetInstance(BSP_I2C_COMP)) {
		if (bsp_i2c_callback.comp.MasterTxCpltCallback)
			bsp_i2c_callback.comp.MasterTxCpltCallback();
	}
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == I2C_GetInstance(BSP_I2C_COMP)) {
		if (bsp_i2c_callback.comp.MasterRxCpltCallback)
			bsp_i2c_callback.comp.MasterRxCpltCallback();
	}
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == I2C_GetInstance(BSP_I2C_COMP)) {
		if (bsp_i2c_callback.comp.SlaveTxCpltCallback)
			bsp_i2c_callback.comp.SlaveTxCpltCallback();
	}
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == I2C_GetInstance(BSP_I2C_COMP)) {
		if (bsp_i2c_callback.comp.SlaveRxCpltCallback)
			bsp_i2c_callback.comp.SlaveRxCpltCallback();
	}
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == I2C_GetInstance(BSP_I2C_COMP)) {
		if (bsp_i2c_callback.comp.ListenCpltCallback)
			bsp_i2c_callback.comp.ListenCpltCallback();
	}
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == I2C_GetInstance(BSP_I2C_COMP)) {
		if (bsp_i2c_callback.comp.MemTxCpltCallback)
			bsp_i2c_callback.comp.MemTxCpltCallback();
	}
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == I2C_GetInstance(BSP_I2C_COMP)) {
		if (bsp_i2c_callback.comp.MemRxCpltCallback)
			bsp_i2c_callback.comp.MemRxCpltCallback();
	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == I2C_GetInstance(BSP_I2C_COMP)) {
		if (bsp_i2c_callback.comp.ErrorCallback)
			bsp_i2c_callback.comp.ErrorCallback();
	}
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c->Instance == I2C_GetInstance(BSP_I2C_COMP)) {
		if (bsp_i2c_callback.comp.AbortCpltCallback)
			bsp_i2c_callback.comp.AbortCpltCallback();
	}
}

/* Exported functions --------------------------------------------------------*/
I2C_HandleTypeDef *BSP_I2C_GetHandle(BSP_I2C_t i2c) {
		switch (i2c) {
		case BSP_I2C_COMP:
			return &hi2c2;
		/*
		case BSP_I2C_XXX:
			return &hi2cX;
		*/
		default:
			return NULL;
	}
}

int8_t BSP_I2C_RegisterCallback(BSP_I2C_t i2c, BSP_I2C_Callback_t type, void (*callback)(void)) {
	if (callback == NULL)
		return -1;
	
	switch (i2c) {
		case BSP_I2C_COMP:
			switch (type) {
				case HAL_I2C_MASTER_TX_CPLT_CB:
					bsp_i2c_callback.comp.MasterTxCpltCallback = callback;
					break;
				case HAL_I2C_MASTER_RX_CPLT_CB:
					bsp_i2c_callback.comp.MasterRxCpltCallback = callback;
					break;
				case HAL_I2C_SLAVE_TX_CPLT_CB:
					bsp_i2c_callback.comp.SlaveTxCpltCallback = callback;
					break;
				case HAL_I2C_SLAVE_RX_CPLT_CB:
					bsp_i2c_callback.comp.SlaveRxCpltCallback = callback;
					break;
				case HAL_I2C_LISTEN_CPLT_CB:
					bsp_i2c_callback.comp.ListenCpltCallback = callback;
					break;
				case HAL_I2C_MEM_TX_CPLT_CB:
					bsp_i2c_callback.comp.MemTxCpltCallback = callback;
					break;
				case HAL_I2C_MEM_RX_CPLT_CB:
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
				case HAL_I2C_MASTER_TX_CPLT_CB:
					bsp_i2c_callback.comp.TxCpltCallback = callback;
					break;
				case HAL_I2C_MASTER_RX_CPLT_CB:
					bsp_i2c_callback.comp.RxCpltCallback = callback;
					break;
				case HAL_I2C_SLAVE_TX_CPLT_CB:
					bsp_i2c_callback.comp.TxRxCpltCallback = callback;
					break;
				case HAL_I2C_SLAVE_RX_CPLT_CB:
					bsp_i2c_callback.comp.TxHalfCpltCallback = callback;
					break;
				case HAL_I2C_LISTEN_CPLT_CB:
					bsp_i2c_callback.comp.RxHalfCpltCallback = callback;
					break;
				case HAL_I2C_MEM_TX_CPLT_CB:
					bsp_i2c_callback.comp.TxRxHalfCpltCallback = callback;
					break;
				case HAL_I2C_MEM_RX_CPLT_CB:
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

