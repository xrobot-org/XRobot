#include "bsp_can.h"

#include "main.h"

typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[8];
} can_raw_rx_t;

typedef struct {
  CAN_TxHeaderTypeDef header;
  uint8_t data[8];
} can_raw_tx_t;

typedef struct {
  void (*fn)(bsp_can_t can, void *);
  void *arg;
} can_callback_t;

extern CAN_HandleTypeDef hcan;

static can_callback_t callback_list[BSP_CAN_NUM][BSP_CAN_CB_NUM];

static bool bsp_can_initd = false;

static uint32_t mailbox[BSP_CAN_NUM];

CAN_HandleTypeDef *bsp_can_get_handle(bsp_can_t can) {
  switch (can) {
    case BSP_CAN_1:
      return &hcan;
    default:
      return NULL;
  }
}

static bsp_can_t can_get(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN)
    return BSP_CAN_1;
  else
    return BSP_CAN_ERR;
}

void bsp_can_init(void) {
  CAN_FilterTypeDef can_filter = {0};

  can_filter.FilterBank = 0;
  can_filter.FilterIdHigh = 0;
  can_filter.FilterIdLow = 0;
  can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
  can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
  can_filter.FilterMaskIdHigh = 0;
  can_filter.FilterMaskIdLow = 0;
  can_filter.FilterActivation = ENABLE;
  can_filter.SlaveStartFilterBank = 14;
  can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;

  HAL_CAN_ConfigFilter(bsp_can_get_handle(BSP_CAN_1), &can_filter);
  HAL_CAN_Start(bsp_can_get_handle(BSP_CAN_1));

  HAL_CAN_ActivateNotification(bsp_can_get_handle(BSP_CAN_1),
                               CAN_IT_RX_FIFO0_MSG_PENDING);
  HAL_CAN_ActivateNotification(bsp_can_get_handle(BSP_CAN_1),
                               CAN_IT_TX_MAILBOX_EMPTY);

  bsp_can_initd = true;
}

static void bsp_can_callback(bsp_can_callback_t cb_type,
                             CAN_HandleTypeDef *hcan) {
  bsp_can_t bsp_can = can_get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    can_callback_t cb = callback_list[bsp_can][cb_type];

    if (cb.fn) {
      cb.fn(bsp_can, cb.arg);
    }
  }
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(CAN_TX_CPLT_CALLBACK, hcan);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(CAN_TX_CPLT_CALLBACK, hcan);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(CAN_TX_CPLT_CALLBACK, hcan);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
  HAL_CAN_ResetError(hcan);
  bsp_can_callback(CAN_TX_CPLT_CALLBACK, hcan);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(CAN_RX_MSG_CALLBACK, hcan);
}

int8_t bsp_can_register_callback(bsp_can_t can, bsp_can_callback_t type,
                                 void (*callback)(bsp_can_t can, void *),
                                 void *callback_arg) {
  assert_param(callback);
  assert_param(type != BSP_CAN_CB_NUM);

  callback_list[can][type].fn = callback;
  callback_list[can][type].arg = callback_arg;
  return BSP_OK;
}

int8_t bsp_can_trans_packet(bsp_can_t can, uint32_t StdId, uint8_t *data) {
  CAN_TxHeaderTypeDef header;
  header.StdId = StdId;
  header.IDE = CAN_ID_STD;
  header.RTR = CAN_RTR_DATA;
  header.TransmitGlobalTime = DISABLE;
  header.DLC = 8;

  HAL_StatusTypeDef res = HAL_CAN_AddTxMessage(bsp_can_get_handle(can), &header,
                                               data, mailbox + can);

  if (res == HAL_OK)
    return BSP_OK;
  else
    return BSP_ERR;
}

int8_t bsp_can_get_msg(bsp_can_t can, uint8_t *data, uint32_t *index) {
  can_raw_rx_t rx;
  HAL_StatusTypeDef res;

  switch (can) {
    case BSP_CAN_1:
      res = HAL_CAN_GetRxMessage(bsp_can_get_handle(BSP_CAN_1),
                                 CAN_FILTER_FIFO0, &rx.header, rx.data);
      break;
    default:
      return BSP_ERR;
  }

  if (res == HAL_OK) {
    *index = rx.header.StdId;
    memcpy(data, rx.data, sizeof(rx.data));
    return BSP_OK;
  }

  return BSP_ERR;
}
