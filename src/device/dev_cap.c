#include "dev_cap.h"

#include <stdbool.h>
#include <string.h>

#include "comp_capacity.h"
#include "comp_utils.h"
#include "om.h"

#define CAP_RES (100) /* 电容数据分辨率 */

#define CAP_CUTOFF_VOLT 4.0f /* 电容截止电压，未接升压模块时要高于电调最低工作电压 */

static void cap_decode(cap_feedback_t *fb, const uint8_t *raw) {
  fb->input_volt = (float)((raw[1] << 8) | raw[0]) / (float)CAP_RES;
  fb->cap_volt = (float)((raw[3] << 8) | raw[2]) / (float)CAP_RES;
  fb->input_curr = (float)((raw[5] << 8) | raw[4]) / (float)CAP_RES;
  fb->target_power = (float)((raw[7] << 8) | raw[6]) / (float)CAP_RES;

  /* 更新电容状态和百分比 */
  fb->percentage = capacity_get_capacitor_remain(fb->cap_volt, fb->input_volt,
                                                 CAP_CUTOFF_VOLT);
}

om_status_t cap_rx_callback(om_msg_t *msg, void *arg) {
  bsp_can_wait_init();

  cap_t *cap = (cap_t *)arg;
  can_rx_item_t *rx = (can_rx_item_t *)msg->buff;

  rx->index -= cap->param->index;

  if (rx->index < DEV_CAP_NUMBER) {
    BaseType_t switch_required;
    xQueueOverwriteFromISR(cap->msgq_feedback, rx, &switch_required);
    portYIELD_FROM_ISR(switch_required);
  }

  return OM_OK;
}

err_t cap_init(cap_t *cap, const cap_param_t *param) {
  cap->msgq_control = xQueueCreate(1, sizeof(can_tx_item_t));
  cap->msgq_feedback = xQueueCreate(1, sizeof(can_rx_item_t));
  cap->param = param;

  om_topic_t *cap_tp =
      om_config_topic(NULL, "VDA", "can_cap", cap_rx_callback, cap);

  bsp_can_register_subscriber(cap->param->can, cap_tp, cap->param->index,
                              cap->param->num);

  if (cap->msgq_control && cap->msgq_feedback)
    return RM_OK;
  else
    return ERR_FAIL;
}

err_t cap_update(cap_t *cap, uint32_t timeout) {
  ASSERT(cap);
  can_rx_item_t rx;
  while (pdPASS ==
         xQueueReceive(cap->msgq_feedback, &rx, pdMS_TO_TICKS(timeout))) {
    cap_decode(&(cap->feedback), rx.data);
    cap->online = 1;
    return RM_OK;
  }
  return ERR_FAIL;
}

err_t cap_control(cap_t *cap, cap_control_t *output) {
  ASSERT(cap);
  ASSERT(output);

  uint16_t pwr_lim = (uint16_t)(output->power_limit * CAP_RES);

  uint8_t data[8] = {0};
  data[0] = (pwr_lim >> 8) & 0xFF;
  data[1] = pwr_lim & 0xFF;

  bsp_can_trans_packet(cap->param->can, DEV_CAP_CTRL_ID_BASE, data,
                       &cap->mailbox, 1);
  return RM_OK;
}

err_t cap_handle_offline(cap_t *cap) {
  ASSERT(cap);
  cap->feedback.cap_volt = 0;
  cap->feedback.input_curr = 0;
  cap->feedback.input_volt = 0;
  cap->feedback.target_power = 0;
  cap->online = 0;
  return RM_OK;
}

err_t cap_pack_ui(const cap_t *cap, ui_cap_t *ui) {
  ui->percentage = cap->feedback.percentage;
  ui->online = cap->online;
  return RM_OK;
}
