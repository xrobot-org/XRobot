/* Includes ------------------------------------------------------------------*/
#include "board\spi.h"

#include "main.h"
#include "spi.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct {
	struct {
		void(*TxCpltCallback)(void);		/* SPI Tx Completed callback */
		void(*RxCpltCallback)(void);		/* SPI Rx Completed callback */
		void(*TxRxCpltCallback)(void);		/* SPI TxRx Completed callback */
		void(*TxHalfCpltCallback)(void);	/* SPI Tx Half Completed callback */
		void(*RxHalfCpltCallback)(void);	/* SPI Rx Half Completed callback */
		void(*TxRxHalfCpltCallback)(void);	/* SPI TxRx Half Completed callback */
		void(*ErrorCallback)(void);			/* SPI Error callback */
		void(*AbortCpltCallback)(void);		/* SPI Abort callback */
	} oled;

	struct {
		void(*TxCpltCallback)(void);
		void(*RxCpltCallback)(void);
		void(*TxRxCpltCallback)(void);
		void(*TxHalfCpltCallback)(void);
		void(*RxHalfCpltCallback)(void);
		void(*TxRxHalfCpltCallback)(void);
		void(*ErrorCallback)(void);
		void(*AbortCpltCallback)(void);
	} imu;
	
	/*
	struct {
		void(*TxCpltCallback)(void);
		void(*RxCpltCallback)(void);
		void(*TxRxCpltCallback)(void);
		void(*TxHalfCpltCallback)(void);
		void(*RxHalfCpltCallback)(void);
		void(*TxRxHalfCpltCallback)(void);
		void(*ErrorCallback)(void);
		void(*AbortCpltCallback)(void);
	} xxx;
	*/
} static bsp_spi_callback;

/* Private function  ---------------------------------------------------------*/
static SPI_TypeDef *UART_GetInstance(BSP_SPI_t uart) {
	switch (uart) {
		case BSP_SPI_OLED:
			return SPI2;
		case BSP_SPI_IMU:
			return SPI1;
		/*
		case BSP_SPI_XXX:
			return SPIX;
		*/
		default:
			return NULL;
	}
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == UART_GetInstance(BSP_SPI_OLED)) {
		if (bsp_spi_callback.oled.TxCpltCallback)
			bsp_spi_callback.oled.TxCpltCallback();
	} else if (hspi->Instance == UART_GetInstance(BSP_SPI_IMU)) {
		if (bsp_spi_callback.imu.TxCpltCallback)
			bsp_spi_callback.imu.TxCpltCallback();
	}
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == UART_GetInstance(BSP_SPI_OLED)) {
		if (bsp_spi_callback.oled.RxCpltCallback)
			bsp_spi_callback.oled.RxCpltCallback();
	} else if (hspi->Instance == UART_GetInstance(BSP_SPI_IMU)) {
		if (bsp_spi_callback.imu.RxCpltCallback)
			bsp_spi_callback.imu.RxCpltCallback();
	}
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == UART_GetInstance(BSP_SPI_OLED)) {
		if (bsp_spi_callback.oled.TxRxCpltCallback)
			bsp_spi_callback.oled.TxRxCpltCallback();
	} else if (hspi->Instance == UART_GetInstance(BSP_SPI_IMU)) {
		if (bsp_spi_callback.imu.TxRxCpltCallback)
			bsp_spi_callback.imu.TxRxCpltCallback();
	}
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == UART_GetInstance(BSP_SPI_OLED)) {
		if (bsp_spi_callback.oled.TxHalfCpltCallback)
			bsp_spi_callback.oled.TxHalfCpltCallback();
	} else if (hspi->Instance == UART_GetInstance(BSP_SPI_IMU)) {
		if (bsp_spi_callback.imu.TxHalfCpltCallback)
			bsp_spi_callback.imu.TxHalfCpltCallback();
	}
}

void HAL_SPI_RxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == UART_GetInstance(BSP_SPI_OLED)) {
		if (bsp_spi_callback.oled.RxHalfCpltCallback)
			bsp_spi_callback.oled.RxHalfCpltCallback();
	} else if (hspi->Instance == UART_GetInstance(BSP_SPI_IMU)) {
		if (bsp_spi_callback.imu.RxHalfCpltCallback)
			bsp_spi_callback.imu.RxHalfCpltCallback();
	}
}

void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == UART_GetInstance(BSP_SPI_OLED)) {
		if (bsp_spi_callback.oled.TxRxHalfCpltCallback)
			bsp_spi_callback.oled.TxRxHalfCpltCallback();
	} else if (hspi->Instance == UART_GetInstance(BSP_SPI_IMU)) {
		if (bsp_spi_callback.imu.TxRxHalfCpltCallback)
			bsp_spi_callback.imu.TxRxHalfCpltCallback();
	}
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == UART_GetInstance(BSP_SPI_OLED)) {
		if (bsp_spi_callback.oled.ErrorCallback)
			bsp_spi_callback.oled.ErrorCallback();
	} else if (hspi->Instance == UART_GetInstance(BSP_SPI_IMU)) {
		if (bsp_spi_callback.imu.ErrorCallback)
			bsp_spi_callback.imu.ErrorCallback();
	}
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == UART_GetInstance(BSP_SPI_OLED)) {
		if (bsp_spi_callback.oled.AbortCpltCallback)
			bsp_spi_callback.oled.AbortCpltCallback();
	} else if (hspi->Instance == UART_GetInstance(BSP_SPI_IMU)) {
		if (bsp_spi_callback.imu.AbortCpltCallback)
			bsp_spi_callback.imu.AbortCpltCallback();
	}
	/* 
	else if (hspi->Instance == XXX_SPI) {
		if (bsp_spi_callback.xxx.AbortCpltCallback)
			bsp_spi_callback.xxx.AbortCpltCallback();
	}
	*/
}

/* Exported functions --------------------------------------------------------*/
SPI_HandleTypeDef *BSP_SPI_GetHandle(BSP_SPI_t spi) {
		switch (spi) {
		case BSP_SPI_OLED:
			return &hspi2;
		case BSP_SPI_IMU:
			return &hspi1;
		/*
		case BSP_SPI_XXX:
			return &hspiX;
		*/
		default:
			return NULL;
	}
}

int8_t BSP_SPI_RegisterCallback(BSP_SPI_t spi, BSP_SPI_Callback_t type, void (*callback)(void)) {
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
		default:
			return -1;
	}
	return 0;
}
