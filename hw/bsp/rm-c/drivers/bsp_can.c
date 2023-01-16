#include "bsp_can.h"

#include "bsp_delay.h"
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

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

static can_callback_t callback_list[BSP_CAN_NUM][BSP_CAN_CB_NUM];

static bool bsp_can_initd = false;

static uint32_t mailbox[BSP_CAN_NUM];

CAN_HandleTypeDef *bsp_can_get_handle(bsp_can_t can) {
  switch (can) {
    case BSP_CAN_2:
      return &hcan2;
    case BSP_CAN_1:
      return &hcan1;
    default:
      return NULL;
  }
}

static bsp_can_t can_get(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN2) {
    return BSP_CAN_2;
  } else if (hcan->Instance == CAN1) {
    return BSP_CAN_1;
  } else {
    return BSP_CAN_ERR;
  }
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

  can_filter.FilterBank = 14;
  can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO1;

  HAL_CAN_ConfigFilter(bsp_can_get_handle(BSP_CAN_2), &can_filter);
  HAL_CAN_Start(bsp_can_get_handle(BSP_CAN_2));

  HAL_CAN_ActivateNotification(bsp_can_get_handle(BSP_CAN_2),
                               CAN_IT_RX_FIFO1_MSG_PENDING);
  HAL_CAN_ActivateNotification(bsp_can_get_handle(BSP_CAN_2),
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

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
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

int8_t bsp_can_trans_packet(bsp_can_t can, bsp_can_format_t format, uint32_t id,
                            uint8_t *data) {
  CAN_TxHeaderTypeDef header;

  if (format == CAN_FORMAT_STD) {
    header.StdId = id;
    header.IDE = CAN_ID_STD;
  } else {
    header.ExtId = id;
    header.IDE = CAN_ID_EXT;
  }

  header.RTR = CAN_RTR_DATA;
  header.TransmitGlobalTime = DISABLE;
  header.DLC = 8;

  uint32_t tsr = READ_REG(bsp_can_get_handle(can)->Instance->TSR);

  while (((tsr & CAN_TSR_TME0) == 0U) && ((tsr & CAN_TSR_TME1) == 0U) &&
         ((tsr & CAN_TSR_TME2) == 0U)) {
    tsr = READ_REG(bsp_can_get_handle(can)->Instance->TSR);
    bsp_delay(1);
  }

  HAL_StatusTypeDef res = HAL_CAN_AddTxMessage(bsp_can_get_handle(can), &header,
                                               data, mailbox + can);

  if (res == HAL_OK) {
    return BSP_OK;
  } else {
    return BSP_ERR;
  }
}

int8_t bsp_can_get_msg(bsp_can_t can, uint8_t *data, uint32_t *index) {
  can_raw_rx_t rx = {};
  HAL_StatusTypeDef res = HAL_OK;

  switch (can) {
    case BSP_CAN_1:
      res = HAL_CAN_GetRxMessage(bsp_can_get_handle(BSP_CAN_1),
                                 CAN_FILTER_FIFO0, &rx.header, rx.data);
      break;
    case BSP_CAN_2:
      res = HAL_CAN_GetRxMessage(bsp_can_get_handle(BSP_CAN_2),
                                 CAN_FILTER_FIFO1, &rx.header, rx.data);
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
