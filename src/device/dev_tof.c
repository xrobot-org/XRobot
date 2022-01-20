#include "dev_tof.h"

#include <stdbool.h>
#include <string.h>

#include "comp_utils.h"
#include "dev_referee.h"

#define TOF_RES (1000) /* TOF数据分辨率 */

void tof_decode(tof_feedback_t *fb, const uint8_t *raw) {
  fb->dist = (float)((raw[2] << 16) | (raw[1] << 8) | raw[0]) / (float)TOF_RES;
  fb->status = raw[3];
  fb->signal_strength = (uint16_t)((raw[5] << 8) | raw[4]);
}

void tof_rx_callback(can_rx_item_t *rx, void *arg) {
  ASSERT(rx);
  ASSERT(arg);

  tof_t *tof = (tof_t *)arg;

  if (rx->index < DEV_TOF_SENSOR_NUMBER) {
    BaseType_t switch_required;
    xQueueOverwriteFromISR(tof->msgq_feedback, rx, &switch_required);
    portYIELD_FROM_ISR(switch_required);
  }
}

err_t tof_init(tof_t *tof, const tof_param_t *param) {
  tof->param = param;
  tof->msgq_feedback = xQueueCreate(1, sizeof(can_rx_item_t));
  bsp_can_register_subscriber(tof->param->can, tof->param->index,
                              tof->param->num, tof_rx_callback, tof);
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
    tof_decode(&(tof->feedback[pack.index]), pack.data);
  }
  return RM_OK;
}

err_t tof_handle_offline(tof_t *tof) {
  ASSERT(tof);
  memset(&(tof->feedback), 0, sizeof(tof->feedback));
  return RM_OK;
}
