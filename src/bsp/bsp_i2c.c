#include "bsp_i2c.h"

#include "comp_utils.h"

static bsp_callback_t callback_list[BSP_I2C_NUM][BSP_I2C_CB_NUM];

static bsp_i2c_t i2c_get(I2C_HandleTypeDef *hi2c) {
  if (hi2c->Instance == I2C3) return BSP_I2C_COMP;
  /*
  else if (hi2c->Instance == I2CX)
                  return BSP_I2C_XXX;
  */
  else
    return BSP_I2C_ERR;
}
static void bsp_i2c_callback(I2C_HandleTypeDef *hi2c,
                             bsp_i2c_callback_t cb_type) {
  bsp_i2c_t bsp_i2c = i2c_get(hi2c);
  if (bsp_i2c != BSP_I2C_ERR) {
    bsp_callback_t cb = callback_list[bsp_i2c][cb_type];
    if (cb.fn) {
      cb.fn(cb.arg);
    }
  }
}
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(hi2c, HAL_I2C_MASTER_TX_CPLT_CB);
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(hi2c, HAL_I2C_MASTER_RX_CPLT_CB);
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(hi2c, HAL_I2C_SLAVE_TX_CPLT_CB);
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(hi2c, HAL_I2C_SLAVE_RX_CPLT_CB);
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(hi2c, HAL_I2C_LISTEN_CPLT_CB);
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(hi2c, HAL_I2C_MEM_TX_CPLT_CB);
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(hi2c, HAL_I2C_MEM_RX_CPLT_CB);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(hi2c, HAL_I2C_ERROR_CB);
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c) {
  bsp_i2c_callback(hi2c, HAL_I2C_ABORT_CPLT_CB);
}

I2C_HandleTypeDef *bsp_i2c_get_handle(bsp_i2c_t i2c) {
  switch (i2c) {
    case BSP_I2C_COMP:
      return &hi2c3;
    /*
    case BSP_I2C_XXX:
            return &hi2cX;
    */
    default:
      return NULL;
  }
}

int8_t bsp_i2c_register_callback(bsp_i2c_t i2c, bsp_i2c_callback_t type,
                                void (*callback)(void *), void *callback_arg) {
  ASSERT(callback);
  callback_list[i2c][type].fn = callback;
  callback_list[i2c][type].arg = callback_arg;
  return BSP_OK;
}
