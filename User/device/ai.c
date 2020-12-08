/*
  AI
*/

/* Includes ----------------------------------------------------------------- */
#include "ai.h"

#include <string.h>

#include "bsp\delay.h"
#include "bsp\uart.h"
#include "component\crc16.h"
#include "component\crc8.h"
#include "component\user_math.h"

/* Private define ----------------------------------------------------------- */
#define AI_HEADER_SOF (0xA5)
#define AI_LEN_RX_BUFF (0xFF)

/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
static volatile uint32_t drop_message = 0;

static AI_t *gai;
static uint8_t rxbuf[AI_LEN_RX_BUFF];

static bool inited = false;

/* Private function  -------------------------------------------------------- */

//???????
static void Ai_RxCpltCallback(void) {
  osThreadFlagsSet(gai->thread_alert, SIGNAL_AI_RAW_REDY);
}

static void Ai_IdleLineCallback(void) {
  HAL_UART_AbortReceive_IT(BSP_UART_GetHandle(BSP_UART_AI));
}

static void Ai_AbortRxCpltCallback(void) {
  osThreadFlagsSet(gai->thread_alert, SIGNAL_AI_RAW_REDY);
}

/* Exported functions ------------------------------------------------------- */
int8_t AI_Init(AI_t *ai, osThreadId_t thread_alert) {
  if (ai == NULL) return DEVICE_ERR_NULL;

  if (inited) return DEVICE_ERR_INITED;

  ai->thread_alert = thread_alert;

  BSP_UART_RegisterCallback(BSP_UART_AI, BSP_UART_RX_CPLT_CB,
                            Ai_RxCpltCallback);
  BSP_UART_RegisterCallback(BSP_UART_AI, BSP_UART_ABORT_RX_CPLT_CB,
                            Ai_AbortRxCpltCallback);
  BSP_UART_RegisterCallback(BSP_UART_AI, BSP_UART_IDLE_LINE_CB,
                            Ai_IdleLineCallback);

  __HAL_UART_ENABLE_IT(BSP_UART_GetHandle(BSP_UART_AI), UART_IT_IDLE);

  gai = ai;
  inited = true;
  return 0;
}

bool AI_WaitDmaCplt(void) {
  return (osThreadFlagsWait(SIGNAL_AI_RAW_REDY, osFlagsWaitAll, 0) == osOK);
}

int8_t ai_Restart(void) {
  __HAL_UART_DISABLE(BSP_UART_GetHandle(BSP_UART_DR16));
  __HAL_UART_ENABLE(BSP_UART_GetHandle(BSP_UART_DR16));
  return 0;
}

int8_t AI_StartReceiving(AI_t *ai) {
  (void)ai;
  if (HAL_UART_Receive_DMA(BSP_UART_GetHandle(BSP_UART_AI), rxbuf,
                           AI_LEN_RX_BUFF) == HAL_OK)
    return DEVICE_OK;
  return DEVICE_ERR;
}

int8_t AI_Parse(AI_t *ai) {
  uint32_t data_length =
      AI_LEN_RX_BUFF -
      __HAL_DMA_GET_COUNTER(BSP_UART_GetHandle(BSP_UART_AI)->hdmarx);

  uint8_t index = 0;

  AI_Header_t *header = (AI_Header_t *)(rxbuf + index);
  index += sizeof(AI_Header_t);
  if (index >= data_length) goto error;

  if (CRC8_Verify((uint8_t *)header, sizeof(AI_Header_t))) goto error;

  if (header->sof != AI_HEADER_SOF) goto error;

  AI_CMDID_t *cmd_id = (AI_CMDID_t *)(rxbuf + index);
  ai->cmd_id = *cmd_id;
  index += sizeof(AI_CMDID_t);
  if (index >= data_length) goto error;

  void *target = (rxbuf + index);
  void *origin;
  size_t size;

  switch (*cmd_id) {
    case AI_CMD_ID_COMMAND:
      origin = &(ai->command);
      size = sizeof(CMD_AI_t);
      break;

    default:
      return DEVICE_ERR;
  }
  index += size;
  if (index >= data_length) goto error;

  index += sizeof(Ai_Tail_t);
  if (index != (data_length - 1)) goto error;

  if (CRC16_Verify((uint8_t *)header, sizeof(AI_Header_t)))
    memcpy(target, origin, size);
  else
    goto error;

  return DEVICE_OK;

error:
  drop_message++;
  return DEVICE_ERR;
}
