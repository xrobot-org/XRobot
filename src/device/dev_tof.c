#include "dev_tof.h"

#include <stdbool.h>
#include <string.h>

#include "comp_utils.h"
#include "dev_referee.h"

#define TOF_RES (1000) /* TOF数据分辨率 */

void tof_decode(tof_feedback_data_t *fb_data, const uint8_t *raw) {
  fb_data->dist =
      (float)((raw[2] << 16) | (raw[1] << 8) | raw[0]) / (float)TOF_RES;
  fb_data->status = raw[3];
  fb_data->signal_strength = (uint16_t)((raw[5] << 8) | raw[4]);
}

void tof_rx_callback(om_msg_t *msg, void *arg) {
  ASSERT(msg);
  ASSERT(arg);

  can_rx_item_t *rx = (can_rx_item_t *)msg->buff;
  tof_t *tof = (tof_t *)arg;

  rx->index -= tof->param->index;

  if (rx->index < DEV_TOF_SENSOR_NUMBER) {
    BaseType_t switch_required;
    xQueueOverwriteFromISR(tof->msgq_feedback, rx, &switch_required);
    portYIELD_FROM_ISR(switch_required);
  }
}

err_t tof_init(tof_t *tof, const tof_param_t *param) {
  bsp_can_wait_init();

  tof->param = param;
  tof->msgq_feedback = xQueueCreate(1, sizeof(can_rx_item_t));

  om_topic_t *tp =
      om_config_topic(NULL, "VDA", "can_tof", tof_rx_callback, tof);

  bsp_can_register_subscriber(tof->param->can, tp, tof->param->index,
                              tof->param->num);
  if (tof->msgq_feedback)
    return RM_OK;
  else
    return ERR_FAIL;
}

err_t tof_update(tof_t *tof, uint32_t timeout) {
  ASSERT(tof);
  can_rx_item_t pack;
  while (pdPASS ==
         xQueueReceive(tof->msgq_feedback, &pack, pdMS_TO_TICKS(timeout))) {
    tof_decode(&(tof->feedback.data[pack.index]), pack.data);
  }
  return RM_OK;
}

err_t tof_handle_offline(tof_t *tof) {
  ASSERT(tof);
  memset(&(tof->feedback), 0, sizeof(tof->feedback));
  return RM_OK;
}
