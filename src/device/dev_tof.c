#include "dev_tof.h"

#include <stdbool.h>
#include <string.h>

#include "comp_utils.h"
#include "dev_referee.h"

#define TOF_ID_BASE_LEFT (0x280)
#define TOF_ID_BASE_RIGHT (0x281)

#define TOF_RES (1000) /* TOF数据分辨率 */

void TOF_Decode(tof_feedback_t *fb, const uint8_t *raw) {
  fb->dist = (float)((raw[2] << 16) | (raw[1] << 8) | raw[0]) / (float)TOF_RES;
  fb->status = raw[3];
  fb->signal_strength = (uint16_t)((raw[5] << 8) | raw[4]);
}

void tof_rx_callback(uint32_t index, uint8_t *data, void *arg) {
  tof_t *tof = (tof_t *)arg;

  if (index == 0) {
    xQueueSendToBack(tof->msgq_feedback, data, 0);
  }
}

err_t tof_init(tof_t *tof) {
  tof->msgq_feedback = xQueueCreate(1, sizeof(CAN_RawTx_t));
  BSP_CAN_RegisterSubscriber(tof->param->can, tof->param->index,
                             tof->param->num, tof_rx_callback, tof);
  if (tof->msgq_feedback)
    return RM_OK;
  else
    return ERR_FAIL;
}

err_t tof_update(tof_t *tof, uint32_t timeout) {
  ASSERT(tof);
  CAN_RawTx_t pack;
  while (pdPASS ==
         xQueueReceive(tof->msgq_feedback, &pack, pdMS_TO_TICKS(timeout))) {
    if (pack.header.StdId == 0) {
      TOF_Decode(&(tof->param.feedback[TOF_SENSOR_LEFT]), pack.data);
    }
    if (pack.header.StdId == 1) {
      TOF_Decode(&(tof->param.feedback[TOF_SENSOR_RIGHT]), pack.data);
    }
  }
  return RM_OK;
}

err_t tof_handle_offline(tof_t *tof) {
  ASSERT(tof);
  tof->param.feedback[TOF_SENSOR_LEFT].dist = 0;
  tof->param.feedback[TOF_SENSOR_LEFT].signal_strength = 0;
  tof->param.feedback[TOF_SENSOR_RIGHT].dist = 0;
  tof->param.feedback[TOF_SENSOR_RIGHT].signal_strength = 0;
  return RM_OK;
}
