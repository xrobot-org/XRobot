#pragma once

#include <stdint.h>

#include "bsp.h"
#include "hal_i2c.h"

/* 要添加使用I2C的新设备，需要先在此添加对应的枚举值 */

/* I2C实体枚举，与设备对应 */
typedef enum {
  BSP_I2C_COMP,
  /* BSP_I2C_XXX,*/
  BSP_I2C_NUM,
  BSP_I2C_ERR,
} bsp_i2c_t;

/* I2C支持的中断回调函数类型，具体参考HAL中定义 */
typedef enum {
  HAL_I2C_MASTER_TX_CPLT_CB,
  HAL_I2C_MASTER_RX_CPLT_CB,
  HAL_I2C_SLAVE_TX_CPLT_CB,
  HAL_I2C_SLAVE_RX_CPLT_CB,
  HAL_I2C_LISTEN_CPLT_CB,
  HAL_I2C_MEM_TX_CPLT_CB,
  HAL_I2C_MEM_RX_CPLT_CB,
  HAL_I2C_ERROR_CB,
  HAL_I2C_ABORT_CPLT_CB,
  BSP_I2C_CB_NUM,
} bsp_i2c_callback_t;

I2C_HandleTypeDef *bsp_i2c_get_handle(bsp_i2c_t i2c);
int8_t bsp_i2c_register_callback(bsp_i2c_t i2c, bsp_i2c_callback_t type,
                                void (*callback)(void *),void *callback_arg);
