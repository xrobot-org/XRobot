#include "dev_can.hpp"

#include <cstddef>

using namespace Device;

std::array<Message::Topic<Can::Pack>*, BSP_CAN_NUM> Can::can_tp_;

static Can::Pack pack;

Can::Can() {
  bsp_can_init();

  for (int i = 0; i < BSP_CAN_NUM; i++) {
    can_tp_[i] = static_cast<Message::Topic<Can::Pack>*>(
        System::Memory::Malloc(sizeof(Message::Topic<Can::Pack>)));
    new (can_tp_[i])
        Message::Topic<Can::Pack>(("dev_can_" + std::to_string(i)).c_str());
  }

  auto rx_callback = [](bsp_can_t can, void* arg) {
    (void)(arg);

    while (bsp_can_get_msg(can, pack.data, &(pack.index)) == BSP_OK) {
      can_tp_[can]->PublishFromISR(pack);
    }
  };

  for (int i = 0; i < BSP_CAN_NUM; i++) {
    bsp_can_register_callback(static_cast<bsp_can_t>(i), CAN_RX_MSG_CALLBACK,
                              rx_callback, NULL);
  }
}

bool Can::SendStdPack(bsp_can_t can, Pack& pack) {
  return bsp_can_trans_packet(can, CAN_FORMAT_STD, pack.index, pack.data) ==
         BSP_OK;
}

bool Can::SendExtPack(bsp_can_t can, Pack& pack) {
  return bsp_can_trans_packet(can, CAN_FORMAT_EXT, pack.index, pack.data) ==
         BSP_OK;
}

bool Can::Subscribe(Message::Topic<Can::Pack>& tp, bsp_can_t can,
                    uint32_t index, uint32_t num) {
  ASSERT(num > 0);

  can_tp_[can]->RangeDivide(tp, sizeof(Pack), offsetof(Pack, index),
                            om_member_size_of(Pack, index), index, num);
  return true;
}
