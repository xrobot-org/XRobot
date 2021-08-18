/* Includes ----------------------------------------------------------------- */
#include "bsp_timer.h"

#include "utils.h"

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
static void (*Timer_Callback[BSP_Timer_NUM][BSP_Timer_CB_NUM])(void);

/* Private function  -------------------------------------------------------- */
static BSP_Timer_t Timer_Get(Timer_HandleTypeDef *hspi) {
  if (hspi->Instance == Timer1)
    return BSP_Timer_IMU;
  else if (hspi->Instance == Timer2)
    return BSP_Timer_OLED;
  /*
  else if (hspi->Instance == TimerX)
                  return BSP_Timer_XXX;
  */
  else
    return BSP_Timer_ERR;
}

void HAL_Timer_TxCpltCallback(Timer_HandleTypeDef *hspi) {
  BSP_Timer_t bsp_spi = Timer_Get(hspi);
  if (bsp_spi != BSP_Timer_ERR) {
    if (Timer_Callback[bsp_spi][BSP_Timer_TX_CPLT_CB]) {
      Timer_Callback[bsp_spi][BSP_Timer_TX_CPLT_CB]();
    }
  }
}

void HAL_Timer_RxCpltCallback(Timer_HandleTypeDef *hspi) {
  BSP_Timer_t bsp_spi = Timer_Get(hspi);
  if (bsp_spi != BSP_Timer_ERR) {
    if (Timer_Callback[Timer_Get(hspi)][BSP_Timer_RX_CPLT_CB])
      Timer_Callback[Timer_Get(hspi)][BSP_Timer_RX_CPLT_CB]();
  }
}

void HAL_Timer_TxRxCpltCallback(Timer_HandleTypeDef *hspi) {
  BSP_Timer_t bsp_spi = Timer_Get(hspi);
  if (bsp_spi != BSP_Timer_ERR) {
    if (Timer_Callback[Timer_Get(hspi)][BSP_Timer_TX_RX_CPLT_CB])
      Timer_Callback[Timer_Get(hspi)][BSP_Timer_TX_RX_CPLT_CB]();
  }
}

void HAL_Timer_TxHalfCpltCallback(Timer_HandleTypeDef *hspi) {
  BSP_Timer_t bsp_spi = Timer_Get(hspi);
  if (bsp_spi != BSP_Timer_ERR) {
    if (Timer_Callback[Timer_Get(hspi)][BSP_Timer_TX_HALF_CPLT_CB])
      Timer_Callback[Timer_Get(hspi)][BSP_Timer_TX_HALF_CPLT_CB]();
  }
}

void HAL_Timer_RxHalfCpltCallback(Timer_HandleTypeDef *hspi) {
  BSP_Timer_t bsp_spi = Timer_Get(hspi);
  if (bsp_spi != BSP_Timer_ERR) {
    if (Timer_Callback[Timer_Get(hspi)][BSP_Timer_RX_HALF_CPLT_CB])
      Timer_Callback[Timer_Get(hspi)][BSP_Timer_RX_HALF_CPLT_CB]();
  }
}

void HAL_Timer_TxRxHalfCpltCallback(Timer_HandleTypeDef *hspi) {
  BSP_Timer_t bsp_spi = Timer_Get(hspi);
  if (bsp_spi != BSP_Timer_ERR) {
    if (Timer_Callback[Timer_Get(hspi)][BSP_Timer_TX_RX_HALF_CPLT_CB])
      Timer_Callback[Timer_Get(hspi)][BSP_Timer_TX_RX_HALF_CPLT_CB]();
  }
}

void HAL_Timer_ErrorCallback(Timer_HandleTypeDef *hspi) {
  BSP_Timer_t bsp_spi = Timer_Get(hspi);
  if (bsp_spi != BSP_Timer_ERR) {
    if (Timer_Callback[Timer_Get(hspi)][BSP_Timer_ERROR_CB])
      Timer_Callback[Timer_Get(hspi)][BSP_Timer_ERROR_CB]();
  }
}

void HAL_Timer_AbortCpltCallback(Timer_HandleTypeDef *hspi) {
  BSP_Timer_t bsp_spi = Timer_Get(hspi);
  if (bsp_spi != BSP_Timer_ERR) {
    if (Timer_Callback[Timer_Get(hspi)][BSP_Timer_ABORT_CPLT_CB])
      Timer_Callback[Timer_Get(hspi)][BSP_Timer_ABORT_CPLT_CB]();
  }
}

/* Exported functions ------------------------------------------------------- */
Timer_HandleTypeDef *BSP_Timer_GetHandle(BSP_Timer_t spi) {
  switch (spi) {
    case BSP_Timer_OLED:
      return &hspi2;
    case BSP_Timer_IMU:
      return &hspi1;
    /*
    case BSP_Timer_XXX:
            return &hspiX;
    */
    default:
      return NULL;
  }
}

int8_t BSP_Timer_RegisterCallback(BSP_Timer_t spi, BSP_Timer_Callback_t type,
                                  void (*callback)(void)) {
  ASSERT(callback);
  Timer_Callback[spi][type] = callback;
  return BSP_OK;
}
