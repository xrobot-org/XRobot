#include "dev_ai.h"

#include <string.h>

#include "bsp_delay.h"
#include "bsp_uart.h"
#include "comp_crc16.h"
#include "comp_crc8.h"
#include "comp_utils.h"

#define AI_CMD_LIMIT (1.0f)
#define AI_LEN_RX_BUFF (sizeof(Protocol_DownPackage_t))

static uint8_t rxbuf[AI_LEN_RX_BUFF];

static bool inited = false;

static void ia_rx_cplt_callback(void *arg) {
  ai_t *ai = arg;
  BaseType_t switch_required;
  xSemaphoreGiveFromISR(ai->sem.recv, &switch_required);
  portYIELD_FROM_ISR(switch_required);
}

static void ia_tx_cplt_callback(void *arg) {
  ai_t *ai = arg;
  BaseType_t switch_required;
  xSemaphoreGiveFromISR(ai->sem.trans, &switch_required);
  portYIELD_FROM_ISR(switch_required);
}

int8_t ai_init(ai_t *ai) {
  ASSERT(ai);
  if (inited) return DEVICE_ERR_INITED;
  inited = true;

  ai->sem.recv = xSemaphoreCreateBinary();
  ai->sem.trans = xSemaphoreCreateBinary();

  xSemaphoreGive(ai->sem.trans);

  BSP_UART_RegisterCallback(BSP_UART_AI, BSP_UART_RX_CPLT_CB,
                            ia_rx_cplt_callback, ai);
  BSP_UART_RegisterCallback(BSP_UART_AI, BSP_UART_TX_CPLT_CB,
                            ia_tx_cplt_callback, ai);
  return DEVICE_OK;
}

int8_t ai_restart(void) {
  __HAL_UART_DISABLE(BSP_UART_GetHandle(BSP_UART_AI));
  __HAL_UART_ENABLE(BSP_UART_GetHandle(BSP_UART_AI));
  return DEVICE_OK;
}

bool ai_start_receiving(ai_t *ai) {
  RM_UNUSED(ai);
  return HAL_UART_Receive_DMA(BSP_UART_GetHandle(BSP_UART_AI), rxbuf,
                              AI_LEN_RX_BUFF) == HAL_OK;
}

bool ai_wait_recv_cplt(ai_t *ai, uint32_t timeout) {
  return xSemaphoreTake(ai->sem.recv, pdMS_TO_TICKS(timeout)) == pdTRUE;
}

bool ai_start_trans(ai_t *ai) {
  size_t len = sizeof(ai->to_host.mcu);
  void *src;
  if (ai->ref_updated) {
    len += sizeof(ai->to_host.ref);
    src = &(ai->to_host);
  } else {
    src = &(ai->to_host.mcu);
  }
  ai->ref_updated = false;
  return (HAL_UART_Transmit_DMA(BSP_UART_GetHandle(BSP_UART_AI), src, len) ==
          HAL_OK);
}

bool ai_wait_trans_cplt(ai_t *ai, uint32_t timeout) {
  return xSemaphoreTake(ai->sem.trans, pdMS_TO_TICKS(timeout)) == pdTRUE;
}

int8_t ai_parse_host(ai_t *ai) {
  if (!crc16_verify((const uint8_t *)&(rxbuf), sizeof(ai->form_host))) {
    ai->online = true;
    memcpy(&(ai->form_host), rxbuf, sizeof(ai->form_host));
    memset(rxbuf, 0, AI_LEN_RX_BUFF);
    return DEVICE_OK;
  }
  return DEVICE_ERR;
}

int8_t ai_handle_offline(ai_t *ai) {
  ai->online = false;
  return DEVICE_OK;
}

int8_t ai_pack_mcu_for_host(ai_t *ai, const quaternion_t *quat) {
  ai->to_host.mcu.id = AI_ID_MCU;
  memcpy(&(ai->to_host.mcu.package.data.quat), (const void *)quat,
         sizeof(*quat));
  ai->to_host.mcu.package.data.notice = 0;
  if (ai->mode == AI_MODE_AUTOAIM)
    ai->to_host.mcu.package.data.notice |= AI_NOTICE_AUTOAIM;
  else if (ai->mode == AI_MODE_HITBUFF)
    ai->to_host.mcu.package.data.notice |= AI_NOTICE_HITBUFF;
  else if (ai->mode == AI_MODE_FULLAUTO)
    ai->to_host.mcu.package.data.notice |= AI_NOTICE_AUTOMATIC;

  ai->to_host.mcu.package.crc16 = crc16_calc(
      (const uint8_t *)&(ai->to_host.mcu.package),
      sizeof(ai->to_host.mcu.package) - sizeof(uint16_t), CRC16_INIT);
  return DEVICE_OK;
}

int8_t ai_pack_ref_for_host(ai_t *ai, const referee_for_ai_t *ref) {
  RM_UNUSED(ref);
  ai->to_host.ref.id = AI_ID_REF;
  ai->to_host.ref.package.data.team = ref->team;
  ai->to_host.ref.package.crc16 = crc16_calc(
      (const uint8_t *)&(ai->to_host.ref.package),
      sizeof(ai->to_host.ref.package) - sizeof(uint16_t), CRC16_INIT);

  ai->ref_updated = false;
  return DEVICE_OK;
}

void ai_pack_ui(ai_ui_t *ui, const ai_t *ai) { ui->mode = ai->mode; }

void ai_pack_cmd(ai_t *ai, cmd_host_t *cmd_host) {
  cmd_host->gimbal_delta.yaw = -ai->form_host.data.gimbal.yaw;
  cmd_host->gimbal_delta.pit = -ai->form_host.data.gimbal.pit;

  cmd_host->fire = (ai->form_host.data.notice & AI_NOTICE_FIRE);
  cmd_host->chassis_move_vec.vx = ai->form_host.data.chassis_move_vec.vx;
  cmd_host->chassis_move_vec.vy = ai->form_host.data.chassis_move_vec.vy;
  cmd_host->chassis_move_vec.wz = ai->form_host.data.chassis_move_vec.wz;
}
