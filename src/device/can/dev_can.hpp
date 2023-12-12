#pragma once

#include <device.hpp>

#include "bsp_can.h"

namespace Device {
class Can {
 public:
  typedef struct {
    uint32_t index;
    uint8_t data[8];
  } Pack;

  Can();

  static bool SendPack(bsp_can_t can, bsp_can_format_t format, Pack& pack);

  static bool SendStdPack(bsp_can_t can, Pack& pack);

  static bool SendExtPack(bsp_can_t can, Pack& pack);

  static bool Subscribe(Message::Topic<Can::Pack>& tp, bsp_can_t can,
                        uint32_t index, uint32_t num);

  static std::array<Message::Topic<Can::Pack>*, BSP_CAN_NUM> can_tp_;
  static std::array<System::Semaphore*, BSP_CAN_NUM> can_sem_;
};
}  // namespace Device
