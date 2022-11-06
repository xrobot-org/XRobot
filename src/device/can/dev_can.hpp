#pragma once

#include <stdbool.h>

#include "FreeRTOS.h"
#include "bsp_can.h"
#include "comp_ahrs.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"

namespace Device {
class Can {
 public:
  typedef struct {
    uint32_t index;
    uint8_t data[8];
  } Pack;

  Can();

  static bool SendPack(bsp_can_t can, Pack& pack);

  static bool Subscribe(Message::Topic<Can::Pack>& tp, bsp_can_t can,
                        uint32_t index, uint32_t num);

  static Message::Topic<Can::Pack>* can_tp_[BSP_CAN_NUM];
  static System::Semaphore* can_sem_[BSP_CAN_NUM];
};
}  // namespace Device
