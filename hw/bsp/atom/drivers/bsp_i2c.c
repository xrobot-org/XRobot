#include "bsp_i2c.h"

#include "bsp_def.h"
#include "main.h"
#include "stm32g4xx_hal_i2c.h"

extern I2C_HandleTypeDef hi2c1;

static bsp_callback_t callback_list[BSP_I2C_NUM][BSP_I2C_CB_NUM];

static bsp_i2c_t i2c_get(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C1) {
    return BSP_I2C_MAGN;
  }
  /*
  else if (hi2c->Instance == I2CX)
                  return BSP_I2C_XXX;
  */
  else {
    return BSP_I2C_ERR;
  }
}

static void bsp_i2c_callback(bsp_i2c_callback_t cb_type,
                             I2C_HandleTypeDef *hi2c) {
  bsp_i2c_t bsp_i2c = i2c_get(hi2c);
  if (bsp_i2c != BSP_I2C_ERR) {
    bsp_callback_t cb = callback_list[bsp_i2c][cb_type];

    if (cb.fn) {
      cb.fn(cb.arg);
    }
  }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(BSP_I2C_RX_CPLT_CB, hi2c);
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(BSP_I2C_TX_CPLT_CB, hi2c);
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(BSP_I2C_RX_CPLT_CB, hi2c);
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(BSP_I2C_TX_CPLT_CB, hi2c);
}

I2C_HandleTypeDef *bsp_i2c_get_handle(bsp_i2c_t i2c) {
  switch (i2c) {
    case BSP_I2C_MAGN:
      return &hi2c1;
    /*
    case BSP_I2C_XXX:
            return &hi2cX;
    */
    default:
      return NULL;
  }
}

bsp_status_t bsp_i2c_register_callback(bsp_i2c_t i2c, bsp_i2c_callback_t type,
                                       void (*callback)(void *),
                                       void *callback_arg) {
  assert_param(callback);
  assert_param(type != BSP_I2C_CB_NUM);

  callback_list[i2c][type].fn = callback;
  callback_list[i2c][type].arg = callback_arg;
  return BSP_OK;
}

bsp_status_t bsp_i2c_transmit(bsp_i2c_t i2c, uint8_t addr, uint8_t *data,
                              size_t size, bool block) {
  if (block) {
    return HAL_I2C_Master_Transmit(bsp_i2c_get_handle(i2c), addr << 1, data,
                                   size, 10) == HAL_OK
               ? BSP_OK
               : BSP_ERR;
  } else {
    return HAL_I2C_Master_Transmit_DMA(bsp_i2c_get_handle(i2c), addr << 1, data,
                                       size) == HAL_OK
               ? BSP_OK
               : BSP_ERR;
  }
}

bsp_status_t bsp_i2c_receive(bsp_i2c_t i2c, uint8_t addr, uint8_t *buff,
                             size_t size, bool block) {
  if (block) {
    return HAL_I2C_Master_Receive(bsp_i2c_get_handle(i2c), addr << 1, buff,
                                  size, 10) == HAL_OK
               ? BSP_OK
               : BSP_ERR;
  } else {
    return HAL_I2C_Master_Receive_DMA(bsp_i2c_get_handle(i2c), addr << 1, buff,
                                      size) == HAL_OK
               ? BSP_OK
               : BSP_ERR;
  }
}

uint8_t bsp_i2c_mem_read_byte(bsp_i2c_t i2c, uint8_t addr, uint8_t reg) {
  uint8_t buff = 0;
  HAL_I2C_Mem_Read(bsp_i2c_get_handle(i2c), addr << 1, reg,
                   I2C_MEMADD_SIZE_8BIT, &buff, 1, 10);
  return buff;
}

bsp_status_t bsp_i2c_mem_write_byte(bsp_i2c_t i2c, uint8_t addr, uint8_t reg,
                                    uint8_t data) {
  return HAL_I2C_Mem_Write(bsp_i2c_get_handle(i2c), addr << 1, reg,
                           I2C_MEMADD_SIZE_8BIT, &data, 1, 10) == HAL_OK
             ? BSP_OK
             : BSP_ERR;
}

bsp_status_t bsp_i2c_mem_read(bsp_i2c_t i2c, uint8_t addr, uint8_t reg,
                              uint8_t *data, size_t size, bool block) {
  if (block) {
    return HAL_I2C_Mem_Read(bsp_i2c_get_handle(i2c), addr << 1, reg,
                            I2C_MEMADD_SIZE_8BIT, data, size, 10) == HAL_OK
               ? BSP_OK
               : BSP_ERR;
  } else {
    return HAL_I2C_Mem_Read_DMA(bsp_i2c_get_handle(i2c), addr << 1, reg,
                                I2C_MEMADD_SIZE_8BIT, data, size) == HAL_OK
               ? BSP_OK
               : BSP_ERR;
  }
}

bsp_status_t bsp_i2c_mem_write(bsp_i2c_t i2c, uint8_t addr, uint8_t reg,
                               uint8_t *buff, size_t size, bool block) {
  if (block) {
    return HAL_I2C_Mem_Write(bsp_i2c_get_handle(i2c), addr << 1, reg,
                             I2C_MEMADD_SIZE_8BIT, buff, size, 10) == HAL_OK
               ? BSP_OK
               : BSP_ERR;
  } else {
    return HAL_I2C_Mem_Write_DMA(bsp_i2c_get_handle(i2c), addr << 1, reg,
                                 I2C_MEMADD_SIZE_8BIT, buff, size) == HAL_OK
               ? BSP_OK
               : BSP_ERR;
  }
}
