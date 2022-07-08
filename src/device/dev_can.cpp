#include "dev_can.hpp"

#include <stdbool.h>
#include <string.h>

#include "comp_utils.hpp"
#include "om.h"

using namespace Device;

can_rx_item_t raw_rx1;
can_rx_item_t raw_rx2;

CAN::CAN() {
  auto can1_callback = [](void *arg) {
    RM_UNUSED(arg);

    while (bsp_can_get_msg(BSP_CAN_1, &raw_rx1) == BSP_OK) {
      om_publish(bsp_can_get_topic(BSP_CAN_1), OM_PRASE_VAR(raw_rx1), true,
                 true);
    }
  };

  auto can2_callback = [](void *arg) {
    RM_UNUSED(arg);

    while (bsp_can_get_msg(BSP_CAN_2, &raw_rx2) == BSP_OK) {
      om_publish(bsp_can_get_topic(BSP_CAN_2), OM_PRASE_VAR(raw_rx2), true,
                 true);
    }
  };

  bsp_can_register_callback(BSP_CAN_1, HAL_CAN_RX_FIFO0_MSG_PENDING_CB,
                            can1_callback, NULL);

  bsp_can_register_callback(BSP_CAN_2, HAL_CAN_RX_FIFO1_MSG_PENDING_CB,
                            can2_callback, NULL);

  bsp_can_init();
}
