#include "dev_can.hpp"

#include "bsp_can.h"

using namespace Device;

std::array<Message::Topic<Can::Pack>*, BSP_CAN_NUM> Can::can_tp_;

std::array<Message::Topic<Can::FDPack>*, BSP_CAN_NUM> Can::canfd_tp_;

std::array<System::Semaphore*, BSP_CAN_NUM> Can::can_sem_;

static std::array<Can::Pack, BSP_CAN_NUM> pack;

static std::array<Can::FDPack, BSP_CAN_NUM> fd_pack;

static bool print_can_pack = false;

Can* Can::self_;

Can::Can() : cmd_(this, CMD, "can"), print_sem_(0), queue_(32) {
  self_ = this;
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

    if (print_can_pack) {
      self_->queue_.Send(pack[can]);
      self_->print_sem_.Post();
    }
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

int Can::CMD(Can* can, int argc, char** argv) {
  if (argc == 1) {
    printf("send    [id]               发送测试数据\r\n");
    printf("monitor [number: 1-32] [timeout] 监控指定数量的can包\r\n");
  } else if (argc == 3 && strcmp(argv[1], "send") == 0) {
    int id = std::stoi(argv[2]);
    Pack pack;
    pack.index = id;
    for (int i = 0; i < 8; i++) {
      pack.data[i] = i;
    }

    can->SendStdPack(BSP_CAN_1, pack);
  } else if (argc == 4 && strcmp(argv[1], "monitor") == 0) {
    uint32_t number = std::stoi(argv[2]);
    uint32_t time = std::stoi(argv[3]);

    if (time > 1000 * 60 * 10) {
      time = 1000 * 60 * 10;
    }

    if (time < 1) {
      time = 1;
    }

    if (number < 1) {
      number = 1;
    }

    if (number > 32) {
      number = 32;
    }

    print_can_pack = true;
    static Pack pack;
    for (uint32_t i = 0; i < time; i += 1) {
      System::Thread::Sleep(1);
      if (self_->queue_.Size() > number) {
        print_can_pack = false;
        break;
      }
    }

    print_can_pack = false;

    while (self_->queue_.Receive(pack)) {
      printf("CanId ID:%08x DATA:%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
             pack.index, pack.data[0], pack.data[1], pack.data[2], pack.data[3],
             pack.data[4], pack.data[5], pack.data[6], pack.data[7]);
    }
  }
  return 0;
}
