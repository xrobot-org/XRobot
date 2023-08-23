#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

/* 要添加使用SPI的新设备，需要先在此添加对应的枚举值 */

/* SPI实体枚举，与设备对应 */
typedef enum {
  BSP_SPI_IMU,
  /* BSP_SPI_XXX,*/
  BSP_SPI_NUM,
  BSP_SPI_ERR,
} bsp_spi_t;

typedef enum {
  BSP_SPI_TX_CPLT_CB,
  BSP_SPI_RX_CPLT_CB,
  BSP_SPI_CB_NUM,
} bsp_spi_callback_t;

bsp_status_t bsp_spi_register_callback(bsp_spi_t spi, bsp_spi_callback_t type,
                                       void (*callback)(void *),
                                       void *callback_arg);
bsp_status_t bsp_spi_transmit_receive(bsp_spi_t spi, uint8_t *recv_data,
                                      const uint8_t *trans_data, size_t size,
                                      bool block);
bsp_status_t bsp_spi_transmit(bsp_spi_t spi, const uint8_t *data, size_t size,
                              bool block);
bsp_status_t bsp_spi_receive(bsp_spi_t spi, uint8_t *buff, size_t size,
                             bool block);
uint8_t bsp_spi_mem_read_byte(bsp_spi_t spi, uint8_t reg);
bsp_status_t bsp_spi_mem_write_byte(bsp_spi_t spi, uint8_t reg, uint8_t data);
bsp_status_t bsp_spi_mem_read(bsp_spi_t spi, uint8_t reg, uint8_t *buff,
                              size_t size, bool block);
bsp_status_t bsp_spi_mem_write(bsp_spi_t spi, uint8_t reg, const uint8_t *buff,
                               size_t size, bool block);

#ifdef __cplusplus
}
#endif
