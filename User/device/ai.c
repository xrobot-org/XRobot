/*
  AI
*/

/* Includes ----------------------------------------------------------------- */
#include "ai.h"

#include <string.h>

#include "bsp/delay.h"
#include "bsp/uart.h"
#include "component/crc16.h"
#include "component/crc8.h"
#include "component/utils.h"

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
static volatile uint32_t drop_message = 0;

static osThreadId_t thread_alert;

static bool inited = false;

/* Private function  -------------------------------------------------------- */

static void Ai_RxCpltCallback(void) {
  osThreadFlagsSet(thread_alert, SIGNAL_AI_RAW_REDY);
}

/* Exported functions ------------------------------------------------------- */
int8_t AI_Init(AI_t *ai) {
  ASSERT(ai);
  if (inited) return DEVICE_ERR_INITED;
  VERIFY((thread_alert = osThreadGetId()) != NULL);

  BSP_UART_RegisterCallback(BSP_UART_AI, BSP_UART_RX_CPLT_CB,
                            Ai_RxCpltCallback);
  inited = true;
  return 0;
}

int8_t AI_Restart(void) {
  __HAL_UART_DISABLE(BSP_UART_GetHandle(BSP_UART_AI));
  __HAL_UART_ENABLE(BSP_UART_GetHandle(BSP_UART_AI));
  return DEVICE_OK;
}

int8_t AI_StartReceiving(AI_t *ai) {
  if (HAL_UART_Receive_DMA(BSP_UART_GetHandle(BSP_UART_AI),
                           (uint8_t *)&(ai->form_host),
                           sizeof(ai->form_host)) == HAL_OK)
    return DEVICE_OK;
  return DEVICE_ERR;
}

bool AI_WaitDmaCplt(void) {
  return (osThreadFlagsWait(SIGNAL_AI_RAW_REDY, osFlagsWaitAll, 0) ==
          SIGNAL_AI_RAW_REDY);
}

int8_t AI_ParseHost(AI_t *ai, CMD_Host_t *cmd_host) {
  UNUSED(cmd_host);
  if (!CRC16_Verify((const uint8_t *)&(ai->form_host), sizeof(ai->form_host)))
    goto error;
  cmd_host->gimbal_delta.pit = ai->form_host.data.gimbal.pit;
  cmd_host->gimbal_delta.yaw = ai->form_host.data.gimbal.yaw;
  cmd_host->gimbal_delta.rol = ai->form_host.data.gimbal.rol;
  cmd_host->fire = (ai->form_host.data.notice & AI_NOTICE_FIRE);
  cmd_host->chassis_move_vec.vx = ai->form_host.data.chassis_move_vec.vx;
  cmd_host->chassis_move_vec.vy = ai->form_host.data.chassis_move_vec.vy;
  cmd_host->chassis_move_vec.wz = ai->form_host.data.chassis_move_vec.wz;
  return DEVICE_OK;

error:
  drop_message++;
  return DEVICE_ERR;
}

int8_t AI_HandleOffline(AI_t *ai, CMD_Host_t *cmd_host) {
  ASSERT(ai);
  ASSERT(cmd_host);

  memset(&(ai->form_host), 0, sizeof(ai->form_host));
  memset(cmd_host, 0, sizeof(*cmd_host));
  return 0;
}

int8_t AI_PackMcu(AI_t *ai, const AHRS_Quaternion_t *quat) {
  ai->to_host.mcu.id = AI_ID_MCU;
  memcpy(&(ai->to_host.mcu.package.data.quat), (const void *)quat,
         sizeof(*quat));
  ai->to_host.mcu.package.data.notice = 0;
  if (ai->status == AI_STATUS_AUTOAIM)
    ai->to_host.mcu.package.data.notice |= AI_NOTICE_AOTUAIM;
  else if (ai->status == AI_STATUS_HITBUFF)
    ai->to_host.mcu.package.data.notice |= AI_NOTICE_HITSWITCH;
  else if (ai->status == AI_STATUS_FULLAUTO)
    ai->to_host.mcu.package.data.notice |= AI_NOTICE_AUTOMATIC;

  ai->to_host.mcu.package.crc16 = CRC16_Calc(
      (const uint8_t *)&(ai->to_host.mcu.package),
      sizeof(ai->to_host.mcu.package) - sizeof(uint16_t), CRC16_INIT);
  return DEVICE_OK;
}

int8_t AI_PackRef(AI_t *ai, const Referee_ForAI_t *ref) {
  UNUSED(ref);
  ai->to_host.ref.id = AI_ID_REF;
  ai->to_host.ref.package.crc16 = CRC16_Calc(
      (const uint8_t *)&(ai->to_host.ref.package),
      sizeof(ai->to_host.ref.package) - sizeof(uint16_t), CRC16_INIT);
  return DEVICE_OK;
}

int8_t AI_StartTrans(AI_t *ai, bool ref_update) {
  if (ref_update) {
    if (HAL_UART_Transmit_DMA(
            BSP_UART_GetHandle(BSP_UART_AI), (uint8_t *)&(ai->to_host),
            sizeof(ai->to_host.ref) + sizeof(ai->to_host.mcu)) == HAL_OK)
      return DEVICE_OK;
    else
      return DEVICE_ERR;
  } else {
    if (HAL_UART_Transmit_DMA(BSP_UART_GetHandle(BSP_UART_AI),
                              (uint8_t *)&(ai->to_host.mcu),
                              sizeof(ai->to_host.mcu)) == HAL_OK)
      return DEVICE_OK;
    else
      return DEVICE_ERR;
  }
}

void AI_PackUi(AI_UI_t *ui, const AI_t *ai) { ui->status = ai->status; }
