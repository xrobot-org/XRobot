/* Includes ------------------------------------------------------------------*/
#include "bsp_spi.h"
#include "main.h"
#include "spi.h"

/* Private define ------------------------------------------------------------*/
#define OLED_SPI SPI1
#define IMU_SPI SPI5
/* #define XXX_SPI SPIX */

/* Private macro -------------------------------------------------------------*/
#define IMU_SPI_NSS_Reset()	HAL_GPIO_WritePin(SPI5_NSS_GPIO_Port, SPI5_NSS_Pin, GPIO_PIN_RESET)
#define IMU_SPI_NSS_Set()	HAL_GPIO_WritePin(SPI5_NSS_GPIO_Port, SPI5_NSS_Pin, GPIO_PIN_SET)

/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static struct {
	 struct {
		void (*TxCpltCallback)(void); /* SPI Tx Completed callback */
		  void (*RxCpltCallback)(void); /* SPI Rx Completed callback */
		  void (*TxRxCpltCallback)(void); /* SPI TxRx Completed callback */
		  void (*TxHalfCpltCallback)(void); /* SPI Tx Half Completed callback */
		  void (*RxHalfCpltCallback)(void); /* SPI Rx Half Completed callback */
		  void (*TxRxHalfCpltCallback)(void); /* SPI TxRx Half Completed callback */
		  void (*ErrorCallback)(void); /* SPI Error callback */
		  void (*AbortCpltCallback)(void); /* SPI Abort callback */
	 } oled;

	 struct {
		  void (*TxCpltCallback)(void); /* SPI Tx Completed callback */
		  void (*RxCpltCallback)(void); /* SPI Rx Completed callback */
		  void (*TxRxCpltCallback)(void); /* SPI TxRx Completed callback */
		  void (*TxHalfCpltCallback)(void); /* SPI Tx Half Completed callback */
		  void (*RxHalfCpltCallback)(void); /* SPI Rx Half Completed callback */
		  void (*TxRxHalfCpltCallback)(void); /* SPI TxRx Half Completed callback */
		  void (*ErrorCallback)(void); /* SPI Error callback */
		  void (*AbortCpltCallback)(void); /* SPI Abort callback */
	 } imu;
} bsp_spi_callback;

/* Private function  ---------------------------------------------------------*/
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
	 if(hspi->Instance == OLED_SPI) {
		if (bsp_spi_callback.oled.TxCpltCallback != NULL) {
			bsp_spi_callback.oled.TxCpltCallback();
		}
	 } else if (hspi->Instance == IMU_SPI) {
		 IMU_SPI_NSS_Set();
		if (bsp_spi_callback.imu.TxCpltCallback != NULL) {
			bsp_spi_callback.imu.TxCpltCallback();
		}
	 }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
	 if(hspi->Instance == OLED_SPI) {
		  if (bsp_spi_callback.oled.RxCpltCallback != NULL) {
				bsp_spi_callback.oled.RxCpltCallback();
		}
	 } else if (hspi->Instance == IMU_SPI) {
		  IMU_SPI_NSS_Set();
		if (bsp_spi_callback.imu.RxCpltCallback != NULL) {
			bsp_spi_callback.imu.RxCpltCallback();
		}
	}
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if(hspi->Instance == OLED_SPI) {
		if (bsp_spi_callback.oled.TxRxCpltCallback != NULL) {
			bsp_spi_callback.oled.TxRxCpltCallback();
		}
	} else if (hspi->Instance == IMU_SPI) {
		if (bsp_spi_callback.imu.TxRxCpltCallback != NULL) {
			bsp_spi_callback.imu.TxRxCpltCallback();
		}
	}
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
	if(hspi->Instance == OLED_SPI) {
		if (bsp_spi_callback.oled.TxHalfCpltCallback != NULL) {
			bsp_spi_callback.oled.TxHalfCpltCallback();
		}
	} else if (hspi->Instance == IMU_SPI) {
		if (bsp_spi_callback.imu.TxHalfCpltCallback != NULL) {
			bsp_spi_callback.imu.TxHalfCpltCallback();
		}
	}
}

void HAL_SPI_RxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
	if(hspi->Instance == OLED_SPI) {
		if (bsp_spi_callback.oled.RxHalfCpltCallback != NULL) {
			bsp_spi_callback.oled.RxHalfCpltCallback();
		}
	} else if (hspi->Instance == IMU_SPI) {
		if (bsp_spi_callback.imu.RxHalfCpltCallback != NULL) {
			bsp_spi_callback.imu.RxHalfCpltCallback();
		}
	}
}

void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
	if(hspi->Instance == OLED_SPI) {
		if (bsp_spi_callback.oled.TxRxHalfCpltCallback != NULL) {
			bsp_spi_callback.oled.TxRxHalfCpltCallback();
		}
	} else if (hspi->Instance == IMU_SPI) {
		if (bsp_spi_callback.imu.TxRxHalfCpltCallback != NULL) {
			bsp_spi_callback.imu.TxRxHalfCpltCallback();
		}
	}
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
	if(hspi->Instance == OLED_SPI) {
		if (bsp_spi_callback.oled.ErrorCallback != NULL) {
			bsp_spi_callback.oled.ErrorCallback();
		}
	} else if (hspi->Instance == IMU_SPI) {
		if (bsp_spi_callback.imu.ErrorCallback != NULL) {
			bsp_spi_callback.imu.ErrorCallback();
		}
	}
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi) {
	if(hspi->Instance == OLED_SPI) {
		if (bsp_spi_callback.oled.AbortCpltCallback != NULL) {
			bsp_spi_callback.oled.AbortCpltCallback();
		}
	} else if (hspi->Instance == IMU_SPI) {
		if (bsp_spi_callback.imu.AbortCpltCallback != NULL) {
			bsp_spi_callback.imu.AbortCpltCallback();
		}
	}
	/* 
	else if (hspi->Instance == XXX_SPI) {
		if (bsp_spi_callback.xxx.AbortCpltCallback != NULL) {
			bsp_spi_callback.xxx.AbortCpltCallback();
		}
	}
	*/
}

/* Exported functions --------------------------------------------------------*/
int BSP_SPI_RegisterCallback(BSP_SPI_t spi, BSP_SPI_Callback_t type, void (*callback)(void)) {
	if (callback == NULL)
		return -1;
	
	switch (spi) {
		case BSP_SPI_IMU:
			switch (type) {
				case BSP_SPI_TX_COMPLETE_CB:
					bsp_spi_callback.imu.TxCpltCallback = callback;
					break;
				case BSP_SPI_RX_COMPLETE_CB:
					bsp_spi_callback.imu.RxCpltCallback = callback;
					break;
				case BSP_SPI_TX_RX_COMPLETE_CB:
					bsp_spi_callback.imu.TxRxCpltCallback = callback;
					break;
				case BSP_SPI_TX_HALF_COMPLETE_CB:
					bsp_spi_callback.imu.TxHalfCpltCallback = callback;
					break;
				case BSP_SPI_RX_HALF_COMPLETE_CB:
					bsp_spi_callback.imu.RxHalfCpltCallback = callback;
					break;
				case BSP_SPI_TX_RX_HALF_COMPLETE_CB:
					bsp_spi_callback.imu.TxRxHalfCpltCallback = callback;
					break;
				case BSP_SPI_ERROR_CB:
					bsp_spi_callback.imu.ErrorCallback = callback;
					break;
				case BSP_SPI_ABORT_CB:
					bsp_spi_callback.imu.AbortCpltCallback = callback;
					break; 
				default:
					return -1;
			}
			break;

		case BSP_SPI_OLED:
			switch (type) {
				case BSP_SPI_TX_COMPLETE_CB:
					bsp_spi_callback.oled.TxCpltCallback = callback;
					break;
				case BSP_SPI_RX_COMPLETE_CB:
					bsp_spi_callback.oled.RxCpltCallback = callback;
					break;
				case BSP_SPI_TX_RX_COMPLETE_CB:
					bsp_spi_callback.oled.TxRxCpltCallback = callback;
					break;
				case BSP_SPI_TX_HALF_COMPLETE_CB:
					bsp_spi_callback.oled.TxHalfCpltCallback = callback;
					break;
				case BSP_SPI_RX_HALF_COMPLETE_CB:
					bsp_spi_callback.oled.RxHalfCpltCallback = callback;
					break;
				case BSP_SPI_TX_RX_HALF_COMPLETE_CB:
					bsp_spi_callback.oled.TxRxHalfCpltCallback = callback;
					break;
				case BSP_SPI_ERROR_CB:
					bsp_spi_callback.oled.ErrorCallback = callback;
					break;
				case BSP_SPI_ABORT_CB:
					bsp_spi_callback.oled.AbortCpltCallback = callback;
					break;
				default:
					return -1;
			}
			break;
		/*	
		case BSP_SPI_XXX:
			switch (type) {
				case BSP_SPI_TX_COMPLETE_CB:
					bsp_spi_callback.xxx.TxCpltCallback = callback;
					break;
				case BSP_SPI_RX_COMPLETE_CB:
					bsp_spi_callback.xxx.RxCpltCallback = callback;
					break;
				case BSP_SPI_TX_RX_COMPLETE_CB:
					bsp_spi_callback.xxx.TxRxCpltCallback = callback;
					break;
				case BSP_SPI_TX_HALF_COMPLETE_CB:
					bsp_spi_callback.xxx.TxHalfCpltCallback = callback;
					break;
				case BSP_SPI_RX_HALF_COMPLETE_CB:
					bsp_spi_callback.xxx.RxHalfCpltCallback = callback;
					break;
				case BSP_SPI_TX_RX_HALF_COMPLETE_CB:
					bsp_spi_callback.xxx.TxRxHalfCpltCallback = callback;
					break;
				case BSP_SPI_ERROR_CB:
					bsp_spi_callback.xxx.ErrorCallback = callback;
					break;
				case BSP_SPI_ABORT_CB:
					bsp_spi_callback.xxx.AbortCpltCallback = callback;
					break; 
				default:
					return -1;
			}
			break;
			*/
	}
	return 0;
}

int BSP_SPI_Transmit(BSP_SPI_t spi, uint8_t *data, uint16_t len) {
	if (data == NULL)
		return -1;
	
	switch (spi) {
		case BSP_SPI_IMU:
			/* Do NOT use hardware NSS. It doesn't implement the same logic. */
			IMU_SPI_NSS_Reset();
			HAL_SPI_Transmit(&hspi5, data, len, 55);
			break;

		case BSP_SPI_OLED:
			HAL_SPI_Transmit(&hspi1, data, len, 55);
			//HAL_SPI_Transmit_DMA(&hspi1, data, len);
			break;
		
		/*
		case BSP_SPI_XXX:
			HAL_SPI_Transmit_DMA(&hspix, data, len);
			break;
		*/
	}
	return 0;
}

int BSP_SPI_Receive(BSP_SPI_t spi, uint8_t *data, uint16_t len) {
	if (data == NULL)
		return -1;

	switch (spi) {
		case BSP_SPI_IMU:
			IMU_SPI_NSS_Reset();
			if (len > 1u) {
				HAL_SPI_Receive_DMA(&hspi5, data, len);
			} else {
				HAL_SPI_Receive(&hspi5, data, len, 55);
			}
			break;

		case BSP_SPI_OLED:
			return -1;
		
		/*
		case BSP_SPI_XXX:
			HAL_SPI_Receive(&hspix, data, len);
			break;
		*/
	}
	return 0;
}
