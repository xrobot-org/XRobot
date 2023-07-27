#include "bsp_can.h"

#include "FreeRTOS.h"
#include "bsp.h"
#include "bsp_uart.h"
#include "main.h"
#include "semphr.h"

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

typedef struct __attribute__((packed)) {
  uint8_t start_frame;
  uint32_t id : 31;
  uint8_t type : 1;
  uint8_t data[8];  // NOLINT(modernize-avoid-c-arrays)
  uint8_t end_frame;
} CanUartPack;

enum UART_PACK_FRAME { START = 0xa5, END = 0Xe3 };

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

static uint8_t *uart_recv_buff_addr[BSP_CAN_EXT_NUM];
static uint8_t uart_recv_buff[BSP_CAN_EXT_NUM][2][2 * sizeof(CanUartPack)];

static can_callback_t callback_list[BSP_CAN_NUM][BSP_CAN_CB_NUM];

static bool bsp_can_initd = false;

static uint32_t mailbox[BSP_CAN_BASE_NUM];

static can_raw_rx_t rx_buff[BSP_CAN_BASE_NUM];
static CAN_TxHeaderTypeDef tx_buff[BSP_CAN_BASE_NUM];
static CanUartPack tx_ext_buff[BSP_CAN_EXT_NUM];

static SemaphoreHandle_t tx_cplt[BSP_CAN_EXT_NUM];

static SemaphoreHandle_t rx_cplt_wait_sem[BSP_CAN_BASE_NUM];

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

bsp_uart_t bsp_ext_can_get_handle(bsp_can_t can) {
  switch (can) {
    case BSP_CAN_3:
      return BSP_UART_CAN3;
    case BSP_CAN_4:
      return BSP_UART_CAN4;
    default:
      return BSP_UART_ERR;
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

static void tx_cplt_callback(void *arg) {
  SemaphoreHandle_t *sem = (SemaphoreHandle_t *)arg;
  BaseType_t px_higher_priority_task_woken = 0;
  xSemaphoreGiveFromISR(sem, &px_higher_priority_task_woken);
  portYIELD_FROM_ISR(px_higher_priority_task_woken);
}

static void rx_callback_fn(void *arg) {
  bsp_can_t can = (uint8_t **)(arg)-uart_recv_buff_addr + BSP_CAN_BASE_NUM;

  int32_t index = 0;
  uint8_t *data = *(uint8_t **)(arg);
  uint32_t len = bsp_uart_get_count(bsp_ext_can_get_handle(can));

  if (uart_recv_buff_addr[can - BSP_CAN_BASE_NUM] ==
      uart_recv_buff[can - BSP_CAN_BASE_NUM][0]) {
    uart_recv_buff_addr[can - BSP_CAN_BASE_NUM] =
        uart_recv_buff[can - BSP_CAN_BASE_NUM][1];
  } else {
    uart_recv_buff_addr[can - BSP_CAN_BASE_NUM] =
        uart_recv_buff[can - BSP_CAN_BASE_NUM][0];
  }

  bsp_uart_abort_receive(bsp_ext_can_get_handle(can));

  bsp_uart_receive(bsp_ext_can_get_handle(can),
                   uart_recv_buff_addr[can - BSP_CAN_BASE_NUM],
                   sizeof(CanUartPack), false);

  while (index <= (int32_t)(len) - (int32_t)(sizeof(CanUartPack))) {
    CanUartPack *pack = (CanUartPack *)(data + index);
    if (pack->start_frame != START || pack->end_frame != END) {
      index++;
      continue;
    } else {
      callback_list[can][CAN_RX_MSG_CALLBACK].fn(
          can, pack->id, pack->data,
          callback_list[can][CAN_RX_MSG_CALLBACK].arg);
      index += sizeof(CanUartPack);
    }
  }
};

void bsp_can_init(void) {
  for (int i = 0; i < BSP_CAN_BASE_NUM; i++) {
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

  for (int i = 0; i < BSP_CAN_EXT_NUM; i++) {
    tx_cplt[i] = xSemaphoreCreateBinary();
    xSemaphoreGive(tx_cplt[i]);
    uart_recv_buff_addr[i] = uart_recv_buff[i][0];
    bsp_uart_receive(bsp_ext_can_get_handle(i + BSP_CAN_BASE_NUM),
                     uart_recv_buff_addr[i], sizeof(CanUartPack), false);
  }

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
  bsp_uart_register_callback(
      BSP_UART_CAN3, BSP_UART_IDLE_LINE_CB, rx_callback_fn,
      &uart_recv_buff_addr[BSP_CAN_3 - BSP_CAN_BASE_NUM]);
  bsp_uart_register_callback(
      BSP_UART_CAN4, BSP_UART_IDLE_LINE_CB, rx_callback_fn,
      &uart_recv_buff_addr[BSP_CAN_4 - BSP_CAN_BASE_NUM]);
  bsp_uart_register_callback(BSP_UART_CAN3, BSP_UART_TX_CPLT_CB,
                             tx_cplt_callback, tx_cplt[0]);
  bsp_uart_register_callback(BSP_UART_CAN4, BSP_UART_TX_CPLT_CB,
                             tx_cplt_callback, tx_cplt[1]);
}

static void can_rx_cb_fn(bsp_can_t can) {
  uint32_t fifo = CAN_FILTER_FIFO0;

  if (can == BSP_CAN_2) {
    fifo = CAN_FILTER_FIFO1;
  }

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
  (void)hcan;
  can_rx_cb_fn(BSP_CAN_1);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  (void)hcan;
  can_rx_cb_fn(BSP_CAN_2);
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

bsp_status_t bsp_ext_can_trans_packet(bsp_can_t can, bsp_can_format_t format,
                                      uint32_t id, uint8_t *data) {
  tx_ext_buff[can - BSP_CAN_BASE_NUM].id = id;
  tx_ext_buff[can - BSP_CAN_BASE_NUM].type = format;
  memcpy(tx_ext_buff[can - BSP_CAN_BASE_NUM].data, data, 8);
  tx_ext_buff[can - BSP_CAN_BASE_NUM].start_frame = START;
  tx_ext_buff[can - BSP_CAN_BASE_NUM].end_frame = END;
  xSemaphoreTake(tx_cplt[can - BSP_CAN_BASE_NUM], UINT32_MAX);
  return bsp_uart_transmit(bsp_ext_can_get_handle(can),
                           (uint8_t *)(&tx_ext_buff[can - BSP_CAN_BASE_NUM]),
                           sizeof(tx_ext_buff[can - BSP_CAN_BASE_NUM]),
                           false) == HAL_OK
             ? BSP_OK
             : BSP_ERR;
}

bsp_status_t bsp_can_trans_packet(bsp_can_t can, bsp_can_format_t format,
                                  uint32_t id, uint8_t *data) {
  if (can >= BSP_CAN_BASE_NUM) {
    return bsp_ext_can_trans_packet(can, format, id, data);
  }

  if (format == CAN_FORMAT_STD) {
    tx_buff[can].StdId = id;
    tx_buff[can].IDE = CAN_ID_STD;
  } else {
    tx_buff[can].ExtId = id;
    tx_buff[can].IDE = CAN_ID_EXT;
  }

  tx_buff[can].RTR = CAN_RTR_DATA;
  tx_buff[can].TransmitGlobalTime = DISABLE;
  tx_buff[can].DLC = 8;

  uint32_t tsr = READ_REG(bsp_can_get_handle(can)->Instance->TSR);

  while (((tsr & CAN_TSR_TME0) == 0U) && ((tsr & CAN_TSR_TME1) == 0U) &&
         ((tsr & CAN_TSR_TME2) == 0U)) {
    xSemaphoreTake(rx_cplt_wait_sem[can], 1);
    tsr = READ_REG(bsp_can_get_handle(can)->Instance->TSR);
  }

  HAL_StatusTypeDef res = HAL_CAN_AddTxMessage(
      bsp_can_get_handle(can), &tx_buff[can], data, mailbox + can);

  if (res == HAL_OK) {
    return BSP_OK;
  } else {
    return BSP_ERR;
  }
}
