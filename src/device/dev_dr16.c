/*
        DR16接收机

*/

#include "dev_dr16.h"

#include <string.h>

#include "FreeRTOS.h"
#include "bsp_uart.h"
#include "task.h"

#define DR16_CH_VALUE_MIN (364u)
#define DR16_CH_VALUE_MID (1024u)
#define DR16_CH_VALUE_MAX (1684u)

static bool inited = false;

static dr16_data_t dma_buff;

static void dr16_rx_cplt_callback(void *arg) {
  dr16_t *dr16 = (dr16_t *)arg;
  BaseType_t switch_required;
  xSemaphoreGiveFromISR(dr16->sem.new, &switch_required);
  portYIELD_FROM_ISR(switch_required);
}

static bool dr16_data_corrupted(const dr16_t *dr16) {
  ASSERT(dr16);

  if ((dr16->data->ch_r_x < DR16_CH_VALUE_MIN) ||
      (dr16->data->ch_r_x > DR16_CH_VALUE_MAX))
    return true;

  if ((dr16->data->ch_r_y < DR16_CH_VALUE_MIN) ||
      (dr16->data->ch_r_y > DR16_CH_VALUE_MAX))
    return true;

  if ((dr16->data->ch_l_x < DR16_CH_VALUE_MIN) ||
      (dr16->data->ch_l_x > DR16_CH_VALUE_MAX))
    return true;

  if ((dr16->data->ch_l_y < DR16_CH_VALUE_MIN) ||
      (dr16->data->ch_l_y > DR16_CH_VALUE_MAX))
    return true;

  if (dr16->data->sw_l == 0) return true;

  if (dr16->data->sw_r == 0) return true;

  return false;
}

int8_t dr16_init(dr16_t *dr16) {
  ASSERT(dr16);
  if (inited) return DEVICE_ERR_INITED;

  dr16->data = &dma_buff;

  dr16->sem.new = xSemaphoreCreateBinary();

  bsp_uart_register_callback(BSP_UART_DR16, BSP_UART_RX_CPLT_CB,
                             dr16_rx_cplt_callback, dr16);

  inited = true;
  return DEVICE_OK;
}

int8_t dr16_restart(void) {
  __HAL_UART_DISABLE(bsp_uart_get_handle(BSP_UART_DR16));
  __HAL_UART_ENABLE(bsp_uart_get_handle(BSP_UART_DR16));
  return DEVICE_OK;
}

int8_t dr16_start_dma_recv(dr16_t *dr16) {
  UNUSED(dr16);

  ASSERT(dr16);
  if (bsp_uart_receive(BSP_UART_DR16, (uint8_t *)&(dma_buff), sizeof(dma_buff),
                       false) == HAL_OK)
    return DEVICE_OK;
  return DEVICE_ERR;
}

bool dr16_wait_dma_cplt(dr16_t *dr16, uint32_t timeout) {
  return xSemaphoreTake(dr16->sem.new, pdMS_TO_TICKS(timeout)) == pdTRUE;
}

int8_t dr16_parse_rc(const dr16_t *dr16, cmd_rc_t *rc) {
  ASSERT(dr16);
  ASSERT(rc);

  if (dr16_data_corrupted(dr16)) {
    return DEVICE_ERR;
  } else {
    memset(rc, 0, sizeof(*rc));
  }

  float full_range = (float)(DR16_CH_VALUE_MAX - DR16_CH_VALUE_MIN);

  rc->ch.r.x = 2 * ((float)dr16->data->ch_r_x - DR16_CH_VALUE_MID) / full_range;
  rc->ch.r.y = 2 * ((float)dr16->data->ch_r_y - DR16_CH_VALUE_MID) / full_range;
  rc->ch.l.x = 2 * ((float)dr16->data->ch_l_x - DR16_CH_VALUE_MID) / full_range;
  rc->ch.l.y = 2 * ((float)dr16->data->ch_l_y - DR16_CH_VALUE_MID) / full_range;

  rc->sw_l = (cmd_switch_pos_t)dr16->data->sw_l;
  rc->sw_r = (cmd_switch_pos_t)dr16->data->sw_r;

  rc->mouse.x = dr16->data->x;
  rc->mouse.y = dr16->data->y;
  rc->mouse.z = dr16->data->z;

  rc->mouse.click.l = dr16->data->press_l;
  rc->mouse.click.r = dr16->data->press_r;

  rc->key = dr16->data->key;

  rc->ch_res = ((float)dr16->data->res - DR16_CH_VALUE_MID) / full_range;
  return DEVICE_OK;
}

int8_t dr16_handle_offline(const dr16_t *dr16, cmd_rc_t *rc) {
  ASSERT(dr16);
  ASSERT(rc);

  RM_UNUSED(dr16);
  memset(rc, 0, sizeof(*rc));
  return DEVICE_OK;
}
