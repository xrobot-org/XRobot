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
  if (ai == NULL) return DEVICE_ERR_NULL;
  if (inited) return DEVICE_ERR_INITED;
  if ((thread_alert = osThreadGetId()) == NULL) return DEVICE_ERR_NULL;

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
                           sizeof(Protocol_AI_t)) == HAL_OK)
    return DEVICE_OK;
  return DEVICE_ERR;
}

bool AI_WaitDmaCplt(void) {
  return (osThreadFlagsWait(SIGNAL_AI_RAW_REDY, osFlagsWaitAll, 0) ==
          SIGNAL_AI_RAW_REDY);
}

int8_t AI_ParseHost(AI_t *ai, CMD_Host_t *cmd_host) {
  (void)cmd_host;
  if (!CRC16_Verify((const uint8_t *)&(ai->form_host), sizeof(Protocol_AI_t)))
    goto error;
  cmd_host->gimbal_delta.pit = ai->form_host.data.gimbal_delta.pit;
  cmd_host->gimbal_delta.yaw = ai->form_host.data.gimbal_delta.yaw;
  cmd_host->gimbal_delta.rol = ai->form_host.data.gimbal_delta.rol;
  cmd_host->fire = (ai->form_host.data.notice & AI_NOTICE_FIRE);
  cmd_host->chassis_speed_setpoint = ai->form_host.data.chassis_speed_setpoint;
  return DEVICE_OK;

error:
  drop_message++;
  return DEVICE_ERR;
}

int8_t AI_HandleOffline(AI_t *ai, CMD_Host_t *cmd_host) {
  if (ai == NULL) return DEVICE_ERR_NULL;
  if (cmd_host == NULL) return DEVICE_ERR_NULL;

  memset(&(ai->form_host), 0, sizeof(Protocol_AI_t));
  memset(cmd_host, 0, sizeof(CMD_Host_t));
  return 0;
}

int8_t AI_PackMCU(AI_t *ai, const AHRS_Quaternion_t *quat) {
  ai->to_host.mcu.id = AI_ID_MCU;
  ai->to_host.mcu.data.quat.q0 = quat->q0;
  ai->to_host.mcu.data.quat.q1 = quat->q1;
  ai->to_host.mcu.data.quat.q2 = quat->q2;
  ai->to_host.mcu.data.quat.q3 = quat->q3;

  ai->to_host.mcu.data.notice = 0;
  if (ai->status == AI_STATUS_AUTOAIM)
    ai->to_host.mcu.data.notice |= AI_NOTICE_AOTUAIM;
  else if (ai->status == AI_STATUS_HITSWITCH)
    ai->to_host.mcu.data.notice |= AI_NOTICE_HITSWITCH;
  else if (ai->status == AI_STATUS_AUTOMATIC)
    ai->to_host.mcu.data.notice |= AI_NOTICE_AUTOMATIC;

  ai->to_host.mcu.crc16 =
      CRC16_Calc((const uint8_t *)&(ai->to_host.mcu),
                 sizeof(Protocol_MCU_t) - sizeof(uint16_t), CRC16_INIT);
  return DEVICE_OK;
}

int8_t AI_PackRef(AI_t *ai, const Referee_t *ref) {
  (void)ref;
  ai->to_host.ref.id = AI_ID_REF;
  ai->to_host.ref.crc16 =
      CRC16_Calc((const uint8_t *)&(ai->to_host.ref),
                 sizeof(Protocol_Referee_t) - sizeof(uint16_t), CRC16_INIT);
  return DEVICE_OK;
}

int8_t AI_StartSend(AI_t *ai, bool ref_update) {
  if (ref_update) {
    if (HAL_UART_Transmit_DMA(
            BSP_UART_GetHandle(BSP_UART_AI), (uint8_t *)&(ai->to_host),
            sizeof(Protocol_MCU_t) + sizeof(Protocol_Referee_t)) == HAL_OK)
      return DEVICE_OK;
    else
      return DEVICE_ERR;
  } else {
    if (HAL_UART_Transmit_DMA(BSP_UART_GetHandle(BSP_UART_AI),
                              (uint8_t *)&(ai->to_host.mcu),
                              sizeof(Protocol_MCU_t)) == HAL_OK)
      return DEVICE_OK;
    else
      return DEVICE_ERR;
  }
}
