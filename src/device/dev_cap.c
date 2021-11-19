#include "dev_cap.h"

#include <stdbool.h>
#include <string.h>

#include "bsp_can.h"
#include "comp_capacity.h"
#include "comp_utils.h"

/* Super capacitor */
#define CAP_FB_ID_BASE (0x211)
#define CAP_CTRL_ID_BASE (0x210)

#define CAP_RES (100) /* 电容数据分辨率 */

#define CAP_CUTOFF_VOLT 12.0f /* 电容截止电压，要高于电调最低工作电压 */

static void Cap_Decode(Cap_Feedback_t *fb, const uint8_t *raw) {
  fb->input_volt = (float)((raw[1] << 8) | raw[0]) / (float)CAP_RES;
  fb->cap_volt = (float)((raw[3] << 8) | raw[2]) / (float)CAP_RES;
  fb->input_curr = (float)((raw[5] << 8) | raw[4]) / (float)CAP_RES;
  fb->target_power = (float)((raw[7] << 8) | raw[6]) / (float)CAP_RES;

  /* 更新电容状态和百分比 */
  fb->percentage = Capacity_GetCapacitorRemain(fb->cap_volt, fb->input_volt,
                                               CAP_CUTOFF_VOLT);
}

Err_t Cap_Init(Cap_t *cap) {
  cap->msgq_control = xQueueCreate(1, sizeof(BSP_CAN_RxItem_t));
  cap->msgq_feedback = xQueueCreate(1, sizeof(BSP_CAN_RxItem_t));

  if (cap->msgq_control && cap->msgq_feedback)
    return RM_OK;
  else
    return ERR_FAIL;
}

Err_t Cap_Update(Cap_t *cap, uint32_t timeout) {
  ASSERT(cap);
  BSP_CAN_RxItem_t pack;
  while (pdPASS ==
         xQueueReceive(cap->msgq_feedback, &pack, pdMS_TO_TICKS(timeout))) {
    if (pack.index == 0) {
      Cap_Decode(&(cap->feedback), pack.data);
    }
  }
  return RM_OK;
}

Err_t Cap_Control(Cap_t *cap, Cap_Control_t *output) {
  ASSERT(cap);
  ASSERT(output);

  uint16_t pwr_lim = (uint16_t)(output->power_limit * CAP_RES);

  BSP_CAN_RxItem_t pack;
  pack.id = CAP_CTRL_ID_BASE;
  pack.data[0] = (pwr_lim >> 8) & 0xFF;
  pack.data[1] = pwr_lim & 0xFF;

  xQueueSendToBack(cap->msgq_control, &pack, 0);
  return RM_OK;
}

Err_t Cap_HandleOffline(Cap_t *cap) {
  ASSERT(cap);
  // TODO
  return RM_OK;
}

Err_t Cap_PackUI(const Cap_t *cap, UI_CapUI_t *ui) {
  ui->percentage = cap->feedback.percentage;
  return RM_OK;
}
