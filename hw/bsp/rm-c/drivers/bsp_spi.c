#include "bsp_spi.h"

#include "comp_utils.h"
#include "hal_spi.h"

static bsp_callback_t callback_list[BSP_SPI_NUM][BSP_SPI_CB_NUM];

static bsp_spi_t spi_get(SPI_HandleTypeDef *hspi) {
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

static void bsp_spi_callback(bsp_spi_callback_t cb_type,
                             SPI_HandleTypeDef *hspi) {
  bsp_spi_t bsp_spi = spi_get(hspi);
  if (bsp_spi != BSP_SPI_ERR) {
    bsp_callback_t cb = callback_list[bsp_spi][cb_type];

    if (cb.fn) {
      cb.fn(cb.arg);
    }
  }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  bsp_spi_callback(BSP_SPI_RX_CPLT_CB, hspi);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
  bsp_spi_callback(BSP_SPI_TX_RX_CPLT_CB, hspi);
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
  bsp_spi_callback(BSP_SPI_TX_HALF_CPLT_CB, hspi);
}

void HAL_SPI_RxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
  bsp_spi_callback(BSP_SPI_RX_HALF_CPLT_CB, hspi);
}

void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
  bsp_spi_callback(BSP_SPI_TX_RX_HALF_CPLT_CB, hspi);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
  bsp_spi_callback(BSP_SPI_ERROR_CB, hspi);
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi) {
  bsp_spi_callback(BSP_SPI_ABORT_CPLT_CB, hspi);
}

SPI_HandleTypeDef *bsp_spi_get_handle(bsp_spi_t spi) {
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

int8_t bsp_spi_register_callback(bsp_spi_t spi, bsp_spi_callback_t type,
                                 void (*callback)(void *), void *callback_arg) {
  ASSERT(callback);
  ASSERT(type != BSP_SPI_CB_NUM);

  callback_list[spi][type].fn = callback;
  callback_list[spi][type].arg = callback_arg;
  return BSP_OK;
}

int8_t bsp_spi_transmit(bsp_spi_t spi, uint8_t *data, size_t size, bool block) {
  if (block) {
    return HAL_SPI_Transmit(bsp_spi_get_handle(spi), data, size, 10) != HAL_OK;
  } else {
    return HAL_SPI_Transmit_DMA(bsp_spi_get_handle(spi), data, size) != HAL_OK;
  }
}

int8_t bsp_spi_receive(bsp_spi_t spi, uint8_t *buff, size_t size, bool block) {
  if (block) {
    return HAL_SPI_Receive(bsp_spi_get_handle(spi), buff, size, 10) != HAL_OK;
  } else {
    return HAL_SPI_Receive_DMA(bsp_spi_get_handle(spi), buff, size) != HAL_OK;
  }
}
