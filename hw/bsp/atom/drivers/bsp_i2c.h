#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

/* 要添加使用I2C的新设备，需要先在此添加对应的枚举值 */

/* I2C实体枚举，与设备对应 */
typedef enum {
  BSP_I2C_MAGN,
  /* BSP_I2C_XXX,*/
  BSP_I2C_NUM,
  BSP_I2C_ERR,
} bsp_i2c_t;

typedef enum {
  BSP_I2C_TX_CPLT_CB,
  BSP_I2C_RX_CPLT_CB,
  BSP_I2C_CB_NUM,
} bsp_i2c_callback_t;

bsp_status_t bsp_i2c_register_callback(bsp_i2c_t i2c, bsp_i2c_callback_t type,
                                       void (*callback)(void *),
                                       void *callback_arg);
bsp_status_t bsp_i2c_transmit(bsp_i2c_t i2c, uint8_t addr, uint8_t *data,
                              size_t size, bool block);
bsp_status_t bsp_i2c_receive(bsp_i2c_t i2c, uint8_t addr, uint8_t *buff,
                             size_t size, bool block);
uint8_t bsp_i2c_mem_read_byte(bsp_i2c_t i2c, uint8_t addr, uint8_t reg);
bsp_status_t bsp_i2c_mem_write_byte(bsp_i2c_t i2c, uint8_t addr, uint8_t reg,
                                    uint8_t data);
bsp_status_t bsp_i2c_mem_read(bsp_i2c_t i2c, uint8_t addr, uint8_t reg,
                              uint8_t *data, size_t size, bool block);
bsp_status_t bsp_i2c_mem_write(bsp_i2c_t i2c, uint8_t addr, uint8_t reg,
                               uint8_t *buff, size_t size, bool block);

#ifdef __cplusplus
}
#endif
