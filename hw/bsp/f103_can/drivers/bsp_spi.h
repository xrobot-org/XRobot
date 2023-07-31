#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

/* 要添加使用SPI的新设备，需要先在此添加对应的枚举值 */

/* SPI实体枚举，与设备对应 */
typedef enum {
  BSP_SPI1,
  /* BSP_SPI_XXX,*/
  BSP_SPI_NUM,
  BSP_SPI_ERR,
} bsp_spi_t;

// TODO：
/* SPI支持的中断回调函数类型，具体参考HAL中定义 */
typedef enum {
  BSP_SPI_TX_CPLT_CB,
  BSP_SPI_RX_CPLT_CB,
  BSP_SPI_CB_NUM,
} bsp_spi_callback_t;

bsp_status_t bsp_spi_register_callback(bsp_spi_t spi, bsp_spi_callback_t type,
                                       void (*callback)(void *),
                                       void *callback_arg);
bsp_status_t bsp_spi_transmit(bsp_spi_t spi, uint8_t *data, size_t size,
                              bool block);
bsp_status_t bsp_spi_receive(bsp_spi_t spi, uint8_t *buff, size_t size,
                             bool block);

#ifdef __cplusplus
}
#endif
