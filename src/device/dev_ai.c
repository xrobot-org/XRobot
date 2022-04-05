#include "dev_ai.h"

#include <string.h>

#include "bsp_delay.h"
#include "bsp_uart.h"
#include "comp_crc16.h"
#include "comp_crc8.h"
#include "comp_utils.h"
#include "dev_term.h"
#include "om.h"

#define AI_CMD_LIMIT (0.08f)
#define AI_CTRL_SENSE (1.0f / 90.0f)
#define AI_LEN_RX_BUFF (sizeof(Protocol_DownPackage_t))
#define AI_LEN_TX_BUFF \
  (sizeof(Protocol_UpPackageMCU_t) + sizeof(Protocol_UpPackageReferee_t))

static uint8_t rxbuf[AI_LEN_RX_BUFF];
static uint8_t txbuf[AI_LEN_TX_BUFF];

static bool inited = false;

void ai_rx_cplt_cb(void *arg) {
  ai_t *ai = (ai_t *)arg;
  BaseType_t switch_required;
  ai_read_host(ai);
  xSemaphoreGiveFromISR(ai->sem.data_ready, &switch_required);
  portYIELD_FROM_ISR(switch_required);
}

int8_t ai_init(ai_t *ai) {
  ASSERT(ai);
  if (inited) return DEVICE_ERR_INITED;
  inited = true;

  bsp_usb_register_callback(BSP_USB_CDC, BSP_USB_RX_CPLT_CB, ai_rx_cplt_cb, ai);

  ai->sem.data_ready = xSemaphoreCreateBinary();

  term_get_ctrl(0xff);

  return DEVICE_OK;
}

int8_t ai_restart(void) {
  __HAL_UART_DISABLE(bsp_uart_get_handle(BSP_UART_AI));
  __HAL_UART_ENABLE(bsp_uart_get_handle(BSP_UART_AI));
  return DEVICE_OK;
}

int8_t ai_read_host(ai_t *ai) {
  RM_UNUSED(ai);

  if (term_read(rxbuf, sizeof(rxbuf)) == sizeof(rxbuf))
    return DEVICE_OK;
  else
    return DEVICE_ERR;
}

bool ai_wait_recv_cplt(ai_t *ai) {
  RM_UNUSED(ai);
  return xSemaphoreTake(ai->sem.data_ready, 0) == pdTRUE;
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

  memcpy(txbuf, src, len);
  return term_write(txbuf, sizeof(txbuf)) == RM_OK;
}

int8_t ai_parse_host(ai_t *ai, uint32_t tick) {
  if (crc16_verify((const uint8_t *)&(rxbuf), sizeof(ai->form_host))) {
    ai->online = true;
    ai->last_online_time = tick;
    memcpy(&(ai->form_host), rxbuf, sizeof(ai->form_host));
    memset(rxbuf, 0, AI_LEN_RX_BUFF);
    return DEVICE_OK;
  }
  return DEVICE_ERR;
}

int8_t ai_handle_offline(ai_t *ai, uint32_t tick) {
  /* 离线移交控制权 */
  if (tick - ai->last_online_time > 50) {
    ai->online = false;
  }
  return DEVICE_OK;
}

int8_t ai_pack_mcu_for_host(ai_t *ai, const quaternion_t *quat) {
  ai->to_host.mcu.id = AI_ID_MCU;
  memcpy(&(ai->to_host.mcu.package.data.quat), (const void *)quat,
         sizeof(*quat));
  ai->to_host.mcu.package.crc16 = crc16_calc(
      (const uint8_t *)&(ai->to_host.mcu.package),
      sizeof(ai->to_host.mcu.package) - sizeof(uint16_t), CRC16_INIT);
  return DEVICE_OK;
}

int8_t ai_pack_ref_for_host(ai_t *ai, const referee_for_ai_t *ref) {
  RM_UNUSED(ref);
  ai->to_host.ref.id = AI_ID_REF;
  ai->to_host.mcu.package.data.ball_speed = ref->ball_speed;
  ai->to_host.ref.package.data.arm = ref->robot_id;
  ai->to_host.ref.package.data.rfid = ref->robot_buff;
  ai->to_host.ref.package.data.team = ref->team;
  ai->to_host.ref.package.data.race = ref->game_type;
  ai->to_host.ref.package.crc16 = crc16_calc(
      (const uint8_t *)&(ai->to_host.ref.package),
      sizeof(ai->to_host.ref.package) - sizeof(uint16_t), CRC16_INIT);

  ai->ref_updated = false;
  return DEVICE_OK;
}

void ai_pack_cmd(ai_t *ai, cmd_host_t *cmd_host, eulr_t *eulr) {
  cmd_host->gimbal_delta.yaw =
      (ai->form_host.data.gimbal.yaw - eulr->yaw) * AI_CTRL_SENSE;
  cmd_host->gimbal_delta.pit =
      (ai->form_host.data.gimbal.pit - eulr->pit) * AI_CTRL_SENSE;
  clampf(&cmd_host->gimbal_delta.yaw, -AI_CMD_LIMIT, AI_CMD_LIMIT);
  clampf(&cmd_host->gimbal_delta.pit, -AI_CMD_LIMIT, AI_CMD_LIMIT);

  cmd_host->fire = (ai->form_host.data.notice & AI_NOTICE_FIRE);
  cmd_host->chassis_move_vec.vx = ai->form_host.data.chassis_move_vec.vx;
  cmd_host->chassis_move_vec.vy = ai->form_host.data.chassis_move_vec.vy;
  cmd_host->chassis_move_vec.wz = ai->form_host.data.chassis_move_vec.wz;
}
