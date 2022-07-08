#include "bsp_can.h"

#include "main.h"

typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[CAN_DATA_SIZE];
} can_raw_rx_t;

typedef struct {
  CAN_TxHeaderTypeDef header;
  uint8_t data[CAN_DATA_SIZE];
} can_raw_tx_t;

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

static om_topic_t *can_1_tp, *can_2_tp;

static bsp_callback_t callback_list[BSP_CAN_NUM][BSP_CAN_CB_NUM];

static SemaphoreHandle_t bsp_can_sem[BSP_CAN_NUM];

static bool bsp_can_initd = false;

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

om_topic_t *bsp_can_get_topic(bsp_can_t can) {
  switch (can) {
    case BSP_CAN_2:
      return can_2_tp;
    case BSP_CAN_1:
      return can_1_tp;
    default:
      return NULL;
  }
}

static bsp_can_t can_get(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN2)
    return BSP_CAN_2;
  else if (hcan->Instance == CAN1)
    return BSP_CAN_1;
  else
    return BSP_CAN_ERR;
}

void bsp_can_init(void) {
  can_1_tp = om_config_topic(NULL, "VA", "can_1_rx");
  can_2_tp = om_config_topic(NULL, "VA", "can_2_rx");

  for (uint32_t i = 0; i < BSP_CAN_NUM; i++) {
    bsp_can_sem[i] = xSemaphoreCreateBinary();
    xSemaphoreGive(bsp_can_sem[i]);
  }

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

  can_filter.FilterBank = 14;
  can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO1;

  HAL_CAN_ConfigFilter(bsp_can_get_handle(BSP_CAN_2), &can_filter);
  HAL_CAN_Start(bsp_can_get_handle(BSP_CAN_2));

  HAL_CAN_ActivateNotification(bsp_can_get_handle(BSP_CAN_2),
                               CAN_IT_RX_FIFO1_MSG_PENDING);

  bsp_can_initd = true;
}

static void bsp_can_callback(bsp_can_callback_t cb_type,
                             CAN_HandleTypeDef *hcan) {
  bsp_can_t bsp_can = can_get(hcan);
  if (bsp_can != BSP_CAN_ERR) {
    bsp_callback_t cb = callback_list[bsp_can][cb_type];

    if (cb.fn) {
      cb.fn(cb.arg);
    }
  }
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_TX_MAILBOX0_CPLT_CB, hcan);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_TX_MAILBOX1_CPLT_CB, hcan);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_TX_MAILBOX2_CPLT_CB, hcan);
}

void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_TX_MAILBOX0_ABORT_CB, hcan);
}

void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_TX_MAILBOX1_ABORT_CB, hcan);
}

void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_TX_MAILBOX2_ABORT_CB, hcan);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_RX_FIFO0_MSG_PENDING_CB, hcan);
}

void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_RX_FIFO0_FULL_CB, hcan);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_RX_FIFO1_MSG_PENDING_CB, hcan);
}

void HAL_CAN_RxFifo1FullCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_RX_FIFO1_FULL_CB, hcan);
}

void HAL_CAN_SleepCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_SLEEP_CB, hcan);
}

void HAL_CAN_WakeUpFromRxMsgCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_WAKEUP_FROM_RX_MSG_CB, hcan);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
  bsp_can_callback(HAL_CAN_ERROR_CB, hcan);
}

int8_t bsp_can_register_callback(bsp_can_t can, bsp_can_callback_t type,
                                 void (*callback)(void *), void *callback_arg) {
  assert_param(callback);
  assert_param(type != BSP_CAN_CB_NUM);

  callback_list[can][type].fn = callback;
  callback_list[can][type].arg = callback_arg;
  return BSP_OK;
}

int8_t bsp_can_trans_packet(bsp_can_t can, uint32_t StdId, uint8_t *data,
                            uint32_t *mailbox, uint32_t timeout) {
  CAN_TxHeaderTypeDef header;
  header.StdId = StdId;
  header.IDE = CAN_ID_STD;
  header.RTR = CAN_RTR_DATA;
  header.TransmitGlobalTime = DISABLE;
  header.DLC = 8;

  if (xSemaphoreTake(bsp_can_sem[can], timeout) != pdTRUE) return BSP_ERR;

  HAL_StatusTypeDef res =
      HAL_CAN_AddTxMessage(bsp_can_get_handle(can), &header, data, mailbox);

  xSemaphoreGive(bsp_can_sem[can]);

  if (res == HAL_OK)
    return BSP_OK;
  else
    return BSP_ERR;
}

int8_t bsp_can_get_msg(bsp_can_t can, can_rx_item_t *item) {
  can_raw_rx_t rx;
  HAL_StatusTypeDef res;

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
    item->index = rx.header.StdId;
    memcpy(item->data, rx.data, sizeof(rx.data));
    return BSP_OK;
  }

  return BSP_ERR;
}

int8_t bsp_can_register_subscriber(bsp_can_t can, om_topic_t *sub,
                                   uint32_t index_id, uint32_t number) {
  if (!bsp_can_initd) return BSP_ERR;
  return om_config_filter(bsp_can_get_topic(can), "R", sub,
                          OM_PRASE_STRUCT(can_rx_item_t, index), index_id,
                          number) != OM_OK;
}
