#include "bsp_can.h"

#include <string.h>

#include "comp_utils.h"
#include "stm32f4xx_hal.h"

static CAN_HandleTypeDef hcan1;
static CAN_HandleTypeDef hcan2;

void can_init(void) {
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 3;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_6TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_7TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = ENABLE;
  hcan1.Init.AutoWakeUp = ENABLE;
  hcan1.Init.AutoRetransmission = ENABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  ASSERT(HAL_CAN_Init(&hcan1) == HAL_OK);

  hcan2.Instance = CAN2;
  ASSERT(HAL_CAN_Init(&hcan2) == HAL_OK);
}

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  if (hcan->Instance == CAN1) {
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PD0     ------> CAN1_RX
    PD1     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    /* USER CODE BEGIN CAN1_MspInit 1 */

    /* USER CODE END CAN1_MspInit 1 */
  } else if (hcan->Instance == CAN2) {
    /* CAN2 clock enable */
    __HAL_RCC_CAN2_CLK_ENABLE();
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN2 GPIO Configuration
    PB5     ------> CAN2_RX
    PB6     ------> CAN2_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CAN2 interrupt Init */
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan) {
  if (hcan->Instance == CAN1) {
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();
    /**CAN1 GPIO Configuration
    PD0     ------> CAN1_RX
    PD1     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_0 | GPIO_PIN_1);

    /* CAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
  } else if (hcan->Instance == CAN2) {
    /* Peripheral clock disable */
    __HAL_RCC_CAN2_CLK_DISABLE();

    /**CAN2 GPIO Configuration
    PB5     ------> CAN2_RX
    PB6     ------> CAN2_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5 | GPIO_PIN_6);

    /* CAN2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN2_RX1_IRQn);
  }
}

typedef struct {
} can_t;

typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[CAN_DATA_SIZE];
} can_rx_raw_t;

typedef struct {
  CAN_TxHeaderTypeDef header;
  uint8_t data[CAN_DATA_SIZE];
} can_tx_raw_t;

static can_rx_raw_t raw_rw;
static can_tx_raw_t raw_tx;

static void can_rx_irq(CAN_HandleTypeDef *hcan) {
  BaseType_t switch_required = pdTRUE;

  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &(raw_rw.header), raw_rw.data);

  can_rx_group_t rx_group = {0};
  can_rx_item_t rx_item = {0};
  QueueHandle_t msgq = {0};
  uint32_t index = raw_rw.header.StdId - rx_group.can_id;
  if (index < rx_group.len) {
    rx_item.index = index;
    memcmp(rx_item.data, raw_rw.data, CAN_DATA_SIZE);
    xQueueSendToBackFromISR(msgq, &rx_item, &switch_required);
  }
  portYIELD_FROM_ISR(switch_required);
}

void CAN1_RX0_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }
void CAN1_RX1_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }
void CAN2_RX0_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan2); }
void CAN2_RX1_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan2); }

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
  UNUSED(hcan);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
  UNUSED(hcan);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
  UNUSED(hcan);
}

void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan) { UNUSED(hcan); }

void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan) { UNUSED(hcan); }

void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan) { UNUSED(hcan); }

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  can_rx_irq(hcan);
}

void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan) { UNUSED(hcan); }

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  can_rx_irq(hcan);
}

void HAL_CAN_RxFifo1FullCallback(CAN_HandleTypeDef *hcan) { UNUSED(hcan); }

void HAL_CAN_SleepCallback(CAN_HandleTypeDef *hcan) { UNUSED(hcan); }

void HAL_CAN_WakeUpFromRxMsgCallback(CAN_HandleTypeDef *hcan) { UNUSED(hcan); }

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) { UNUSED(hcan); }

QueueHandle_t can_register_rx_group(const can_rx_group_t *group) {
  UNUSED(group);
  return NULL;
}
