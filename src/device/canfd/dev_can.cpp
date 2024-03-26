#include "dev_can.hpp"

#include "bsp_can.h"

using namespace Device;

std::array<Message::Topic<Can::Pack>*, BSP_CAN_NUM> Can::can_tp_;

std::array<Message::Topic<Can::FDPack>*, BSP_CAN_NUM> Can::canfd_tp_;

std::array<System::Semaphore*, BSP_CAN_NUM> Can::can_sem_;

static std::array<Can::Pack, BSP_CAN_NUM> pack;

static std::array<Can::FDPack, BSP_CAN_NUM> fd_pack;

Can::Can() {
  for (int i = 0; i < BSP_CAN_NUM; i++) {
    can_tp_[i] =
        new Message::Topic<Can::Pack>(("dev_can_" + std::to_string(i)).c_str());
    canfd_tp_[i] = new Message::Topic<Can::FDPack>(
        ("dev_canfd_" + std::to_string(i)).c_str());
    can_sem_[i] = new System::Semaphore(true);
  }

  auto rx_callback = [](bsp_can_t can, uint32_t id, uint8_t* data, void* arg) {
    XB_UNUSED(arg);

    pack[can].index = id;

    memcpy(pack[can].data, data, sizeof(pack[can].data));

    can_tp_[can]->Publish(pack[can]);
  };

  auto fd_rx_callback = [](bsp_can_t can, uint32_t id, uint8_t* data,
                           void* arg) {
    XB_UNUSED(arg);

    fd_pack[can].index = id;

    memcpy(&fd_pack[can].info, data, sizeof(bsp_canfd_data_t));

    canfd_tp_[can]->Publish(fd_pack[can]);
  };

  for (int i = 0; i < BSP_CAN_NUM; i++) {
    bsp_can_register_callback(static_cast<bsp_can_t>(i), CAN_RX_MSG_CALLBACK,
                              rx_callback, NULL);
    bsp_can_register_callback(static_cast<bsp_can_t>(i), CANFD_RX_MSG_CALLBACK,
                              fd_rx_callback, NULL);
  }

  bsp_can_init();
}

bool Can::SendPack(bsp_can_t can, bsp_can_format_t format, Pack& pack) {
  can_sem_[can]->Wait(UINT32_MAX);
  bool ans = bsp_can_trans_packet(can, format, pack.index, pack.data) == BSP_OK;
  can_sem_[can]->Post();
  return ans;
}

bool Can::SendStdPack(bsp_can_t can, Pack& pack) {
  can_sem_[can]->Wait(UINT32_MAX);
  bool ans = bsp_can_trans_packet(can, CAN_FORMAT_STD, pack.index, pack.data) ==
             BSP_OK;
  can_sem_[can]->Post();
  return ans;
}

bool Can::SendExtPack(bsp_can_t can, Pack& pack) {
  can_sem_[can]->Wait(UINT32_MAX);
  bool ans = bsp_can_trans_packet(can, CAN_FORMAT_EXT, pack.index, pack.data) ==
             BSP_OK;
  can_sem_[can]->Post();
  return ans;
}

bool Can::SendFDPack(bsp_can_t can, bsp_can_format_t format, uint32_t id,
                     uint8_t* data, size_t size) {
  can_sem_[can]->Wait(UINT32_MAX);
  bool ans = bsp_canfd_trans_packet(can, format, id, data, size) == BSP_OK;
  can_sem_[can]->Post();
  return ans;
}

bool Can::SendFDStdPack(bsp_can_t can, uint32_t id, uint8_t* data,
                        size_t size) {
  can_sem_[can]->Wait(UINT32_MAX);
  bool ans =
      bsp_canfd_trans_packet(can, CAN_FORMAT_STD, id, data, size) == BSP_OK;
  can_sem_[can]->Post();
  return ans;
}

bool Can::SendFDExtPack(bsp_can_t can, uint32_t id, uint8_t* data,
                        size_t size) {
  can_sem_[can]->Wait(UINT32_MAX);
  bool ans =
      bsp_canfd_trans_packet(can, CAN_FORMAT_EXT, id, data, size) == BSP_OK;
  can_sem_[can]->Post();
  return ans;
}

bool Can::Subscribe(Message::Topic<Can::Pack>& tp, bsp_can_t can,
                    uint32_t index, uint32_t num) {
  XB_ASSERT(num > 0);

  can_tp_[can]->RangeDivide(tp, sizeof(Pack), offsetof(Pack, index),
                            om_member_size_of(Pack, index), index, num);
  return true;
}

bool Can::SubscribeFD(Message::Topic<Can::FDPack>& tp, bsp_can_t can,
                      uint32_t index, uint32_t num) {
  XB_ASSERT(num > 0);

  canfd_tp_[can]->RangeDivide(tp, sizeof(FDPack), offsetof(FDPack, index),
                              om_member_size_of(Pack, index), index, num);
  return true;
}
