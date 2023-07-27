#include "bsp_can.h"

#include "FreeRTOS.h"
#include "main.h"
#include "semphr.h"
#include "task.h"

typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[8];
} can_raw_rx_t;

typedef struct {
  CAN_TxHeaderTypeDef header;
  uint8_t data[8];
} can_raw_tx_t;

typedef struct {
  void (*fn)(bsp_can_t can, uint32_t id, uint8_t *data, void *arg);
  void *arg;
} can_callback_t;

extern CAN_HandleTypeDef hcan;

static can_callback_t callback_list[BSP_CAN_NUM][BSP_CAN_CB_NUM];

static bool bsp_can_initd = false;

static uint32_t mailbox[BSP_CAN_NUM];

static can_raw_rx_t rx_buff[BSP_CAN_NUM];

static SemaphoreHandle_t rx_cplt_wait_sem[BSP_CAN_NUM];

CAN_HandleTypeDef *bsp_can_get_handle(bsp_can_t can) {
  switch (can) {
    case BSP_CAN_1:
      return &hcan;
    default:
      return NULL;
  }
}

static bsp_can_t can_get(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN) {
    return BSP_CAN_1;
  } else {
    return BSP_CAN_ERR;
  }
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
  BaseType_t px_higher_priority_task_woken = 0;
  xSemaphoreGiveFromISR(rx_cplt_wait_sem[can_get(hcan)],
                        &px_higher_priority_task_woken);
  if (px_higher_priority_task_woken != pdFALSE) {
    portYIELD();
  }
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
  BaseType_t px_higher_priority_task_woken = 0;
  xSemaphoreGiveFromISR(rx_cplt_wait_sem[can_get(hcan)],
                        &px_higher_priority_task_woken);
  if (px_higher_priority_task_woken != pdFALSE) {
    portYIELD();
  }
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
  BaseType_t px_higher_priority_task_woken = 0;
  xSemaphoreGiveFromISR(rx_cplt_wait_sem[can_get(hcan)],
                        &px_higher_priority_task_woken);
  if (px_higher_priority_task_woken != pdFALSE) {
    portYIELD();
  }
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
  HAL_CAN_ResetError(hcan);
  BaseType_t px_higher_priority_task_woken = 0;
  xSemaphoreGiveFromISR(rx_cplt_wait_sem[can_get(hcan)],
                        &px_higher_priority_task_woken);
  if (px_higher_priority_task_woken != pdFALSE) {
    portYIELD();
  }
}

void bsp_can_init(void) {
  for (int i = 0; i < BSP_CAN_NUM; i++) {
    rx_cplt_wait_sem[i] = xSemaphoreCreateBinary();
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
  HAL_CAN_ActivateNotification(bsp_can_get_handle(BSP_CAN_1),
                               CAN_IT_TX_MAILBOX_EMPTY);

  bsp_can_initd = true;
}

static void can_rx_cb_fn(bsp_can_t can) {
  uint32_t fifo = CAN_FILTER_FIFO0;

  if (callback_list[can][CAN_RX_MSG_CALLBACK].fn) {
    while (HAL_CAN_GetRxMessage(bsp_can_get_handle(can), fifo,
                                &rx_buff[can].header,
                                rx_buff[can].data) == HAL_OK) {
      if (rx_buff[can].header.IDE == CAN_ID_STD) {
        callback_list[can][CAN_RX_MSG_CALLBACK].fn(
            can, rx_buff[can].header.StdId, rx_buff[can].data,
            callback_list[can][CAN_RX_MSG_CALLBACK].arg);
      } else {
        callback_list[can][CAN_RX_MSG_CALLBACK].fn(
            can, rx_buff[can].header.ExtId, rx_buff[can].data,
            callback_list[can][CAN_RX_MSG_CALLBACK].arg);
      }
    }
  }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  can_rx_cb_fn(can_get(hcan));
}

bsp_status_t bsp_can_register_callback(
    bsp_can_t can, bsp_can_callback_t type,
    void (*callback)(bsp_can_t can, uint32_t id, uint8_t *data, void *arg),
    void *callback_arg) {
  assert_param(callback);
  assert_param(type != BSP_CAN_CB_NUM);

  callback_list[can][type].fn = callback;
  callback_list[can][type].arg = callback_arg;
  return BSP_OK;
}

bsp_status_t bsp_can_trans_packet(bsp_can_t can, bsp_can_format_t format,
                                  uint32_t id, uint8_t *data) {
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
    xSemaphoreTake(rx_cplt_wait_sem[can], 1);
    tsr = READ_REG(bsp_can_get_handle(can)->Instance->TSR);
  }

  HAL_StatusTypeDef res = HAL_CAN_AddTxMessage(bsp_can_get_handle(can), &header,
                                               data, mailbox + can);

  if (res == HAL_OK) {
    return BSP_OK;
  } else {
    return BSP_ERR;
  }
}

bsp_status_t bsp_can_get_msg(bsp_can_t can, uint8_t *data, uint32_t *index) {
  can_raw_rx_t rx = {};
  HAL_StatusTypeDef res = HAL_OK;

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
