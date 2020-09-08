/* Includes ------------------------------------------------------------------*/
#include "bsp\spi.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static void (*SPI_Callback[BSP_SPI_NUM][BSP_SPI_CB_NUM])(void);

/* Private function  ---------------------------------------------------------*/
static BSP_SPI_t SPI_Get(SPI_HandleTypeDef *hspi) {
  if (hspi->Instance == SPI1)
    return BSP_SPI_IMU;
  else if (hspi->Instance == SPI2)
    return BSP_SPI_OLED;
  /*
  else if (hspi->Instance == SPIX)
                  return BSP_SPI_XXX;
  */
  else
    return BSP_SPI_ERR;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  BSP_SPI_t bsp_spi = SPI_Get(hspi);
  if (bsp_spi != BSP_SPI_ERR) {
    if (SPI_Callback[bsp_spi][BSP_SPI_TX_CPLT_CB]) {
      SPI_Callback[bsp_spi][BSP_SPI_TX_CPLT_CB]();
    }
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  BSP_SPI_t bsp_spi = SPI_Get(hspi);
  if (bsp_spi != BSP_SPI_ERR) {
    if (SPI_Callback[SPI_Get(hspi)][BSP_SPI_RX_CPLT_CB])
      SPI_Callback[SPI_Get(hspi)][BSP_SPI_RX_CPLT_CB]();
  }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
  BSP_SPI_t bsp_spi = SPI_Get(hspi);
  if (bsp_spi != BSP_SPI_ERR) {
    if (SPI_Callback[SPI_Get(hspi)][BSP_SPI_TX_RX_CPLT_CB])
      SPI_Callback[SPI_Get(hspi)][BSP_SPI_TX_RX_CPLT_CB]();
  }
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
  BSP_SPI_t bsp_spi = SPI_Get(hspi);
  if (bsp_spi != BSP_SPI_ERR) {
    if (SPI_Callback[SPI_Get(hspi)][BSP_SPI_TX_HALF_CPLT_CB])
      SPI_Callback[SPI_Get(hspi)][BSP_SPI_TX_HALF_CPLT_CB]();
  }
}

void HAL_SPI_RxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
  BSP_SPI_t bsp_spi = SPI_Get(hspi);
  if (bsp_spi != BSP_SPI_ERR) {
    if (SPI_Callback[SPI_Get(hspi)][BSP_SPI_RX_HALF_CPLT_CB])
      SPI_Callback[SPI_Get(hspi)][BSP_SPI_RX_HALF_CPLT_CB]();
  }
}

void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
  BSP_SPI_t bsp_spi = SPI_Get(hspi);
  if (bsp_spi != BSP_SPI_ERR) {
    if (SPI_Callback[SPI_Get(hspi)][BSP_SPI_TX_RX_HALF_CPLT_CB])
      SPI_Callback[SPI_Get(hspi)][BSP_SPI_TX_RX_HALF_CPLT_CB]();
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
  BSP_SPI_t bsp_spi = SPI_Get(hspi);
  if (bsp_spi != BSP_SPI_ERR) {
    if (SPI_Callback[SPI_Get(hspi)][BSP_SPI_ERROR_CB])
      SPI_Callback[SPI_Get(hspi)][BSP_SPI_ERROR_CB]();
  }
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi) {
  BSP_SPI_t bsp_spi = SPI_Get(hspi);
  if (bsp_spi != BSP_SPI_ERR) {
    if (SPI_Callback[SPI_Get(hspi)][BSP_SPI_ABORT_CPLT_CB])
      SPI_Callback[SPI_Get(hspi)][BSP_SPI_ABORT_CPLT_CB]();
  }
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

int8_t BSP_SPI_RegisterCallback(BSP_SPI_t spi, BSP_SPI_Callback_t type,
                                void (*callback)(void)) {
  if (callback == NULL) return BSP_ERR_NULL;
  SPI_Callback[spi][type] = callback;
  return BSP_OK;
}
