#include <comp_utils.hpp>
#include <module.hpp>

#include "bsp_dns_client.h"
#include "bsp_time.h"
#include "bsp_udp_client.h"
#include "comp_crc8.hpp"
#include "dev_canfd.hpp"
#include "udp_param.h"
#include "wearlab.hpp"

namespace Module {
class UartToUDP {
 public:
  UartToUDP() {
    self_ = this;
    om_fifo_create(&udp_tx_fifo_, new uint8_t[512], 512, sizeof(uint8_t));

    bsp_dns_addr_t addrs;
    while (true) {
      auto ans = bsp_dns_prase_domain(UDP_DOMAIN, &addrs);
      if (ans != BSP_OK) {
        OMLOG_ERROR("Prase domain failed:%s", UDP_DOMAIN);
        OMLOG_WARNING("Sleep 5s to wait network ready");
        System::Thread::Sleep(5000);
      } else {
        OMLOG_PASS("Prase domain done:%s", UDP_DOMAIN);
        OMLOG_NOTICE("Server ip:%s", &addrs);
        break;
      }
    }

    bsp_udp_client_init(&udp_client_, UDP_PORT,
                        reinterpret_cast<char*>(&addrs));

    auto udp_rx_cb = [](void* arg, void* buff, uint32_t size) {
      XB_UNUSED(arg);

      if (*reinterpret_cast<uint8_t*>(buff) != 0xa5) {
        return;
      }

      if (!Component::CRC8::Verify(static_cast<uint8_t*>(buff), size)) {
        return;
      }

      auto header = static_cast<Device::WearLab::UdpDataHeader*>(buff);

      if (header->fd) {
        Device::Can::SendFDExtPack(static_cast<bsp_can_t>(header->area_id),
                                   header->can_header.raw,
                                   reinterpret_cast<uint8_t*>(header) +
                                       sizeof(Device::WearLab::UdpDataHeader),
                                   size);
      } else {
        Device::Can::Pack pack = {.index = header->can_header.raw};
        memcpy(pack.data,
               reinterpret_cast<uint8_t*>(header) +
                   sizeof(Device::WearLab::UdpDataHeader),
               8);
        Device::Can::SendExtPack(static_cast<bsp_can_t>(header->area_id), pack);
      }
    };

    bsp_udp_client_register_callback(&udp_client_, BSP_UDP_RX_CPLT_CB,
                                     udp_rx_cb, this);

    auto udp_rx_thread_fn = [](UartToUDP* uart_udp) {
      bsp_udp_client_start(&uart_udp->udp_client_);
      while (true) {
        System::Thread::Sleep(UINT32_MAX);
      }
    };

    auto udp_tx_thread_fn = [](UartToUDP* uart_udp) {
      uint8_t tx_buff[512];

      while (true) {
        uart_udp->udp_tx_sem_.Lock();

        uart_udp->udp_tx_mutex_.Lock();

        auto size = om_fifo_readable_item_count(&uart_udp->udp_tx_fifo_);
        om_fifo_reads(&uart_udp->udp_tx_fifo_, tx_buff, size);

        uart_udp->udp_tx_mutex_.Unlock();

        bsp_udp_client_transmit(&uart_udp->udp_client_, tx_buff, size);
      }
    };

    Message::Topic<Device::Can::FDPack>* fd_tp[BSP_CAN_NUM];
    Message::Topic<Device::Can::Pack>* tp[BSP_CAN_NUM];

    auto canfd_rx_fun = [](Device::Can::FDPack& pack, uint8_t* can) {
      if (*can >= 0) {
        return false;
      }

      static uint8_t udp_tx_buff[BSP_CAN_NUM][256] = {};

      auto header =
          reinterpret_cast<Device::WearLab::UdpDataHeader*>(udp_tx_buff[*can]);

      header->time = bsp_time_get();
      header->prefix = 0xa5;
      header->can_header.raw = pack.index;
      header->area_id = *can;
      header->data_len = pack.info.size;
      header->fd = true;

      memcpy(udp_tx_buff[*can] + sizeof(Device::WearLab::UdpDataHeader),
             pack.info.data, pack.info.size);
      *(udp_tx_buff[*can] + sizeof(Device::WearLab::UdpDataHeader) +
        pack.info.size) =
          Component::CRC8::Calculate(
              udp_tx_buff[*can], sizeof(*header) + pack.info.size, CRC8_INIT);
      self_->udp_tx_mutex_.Lock();
      om_fifo_writes(
          &self_->udp_tx_fifo_, header,
          sizeof(Device::WearLab::UdpDataHeader) + pack.info.size + 1);
      self_->udp_tx_mutex_.Unlock();
      self_->udp_tx_sem_.Unlock();
      return true;
    };

    auto can_rx_fun = [](Device::Can::Pack& pack, uint8_t* can) {
      if (*can >= 0) {
        return false;
      }

      static uint8_t udp_tx_buff[BSP_CAN_NUM][256] = {};

      auto header =
          reinterpret_cast<Device::WearLab::UdpDataHeader*>(udp_tx_buff[*can]);

      header->time = bsp_time_get();
      header->prefix = 0xa5;
      header->can_header.raw = pack.index;
      header->area_id = *can;
      header->data_len = 8;
      header->fd = false;

      memcpy(udp_tx_buff[*can] + sizeof(Device::WearLab::UdpDataHeader),
             pack.data, 8);
      *(udp_tx_buff[*can] + sizeof(Device::WearLab::UdpDataHeader) + 8) =
          Component::CRC8::Calculate(udp_tx_buff[*can], sizeof(*header) + 8,
                                     CRC8_INIT);
      self_->udp_tx_mutex_.Lock();
      om_fifo_writes(&self_->udp_tx_fifo_, header,
                     sizeof(Device::WearLab::UdpDataHeader) + 9);
      self_->udp_tx_mutex_.Unlock();
      self_->udp_tx_sem_.Unlock();
      return true;
    };

    for (int i = 0; i < BSP_CAN_NUM; i++) {
      can_id_[i] = i;

      fd_tp[i] = new Message::Topic<Device::Can::FDPack>(
          (std::string("trans_canfd") + std::to_string(i)).c_str());
      tp[i] = new Message::Topic<Device::Can::Pack>(
          (std::string("trans_can") + std::to_string(i)).c_str());

      Device::Can::SubscribeFD(*fd_tp[i], static_cast<bsp_can_t>(i), 0,
                               UINT32_MAX);

      Device::Can::Subscribe(*tp[i], static_cast<bsp_can_t>(i), 0, UINT32_MAX);

      fd_tp[i]->RegisterCallback(canfd_rx_fun, &can_id_[i]);
      tp[i]->RegisterCallback(can_rx_fun, &can_id_[i]);
    }

    udp_rx_thread_.Create(udp_rx_thread_fn, this, "udp_rx_thread", 512,
                          System::Thread::HIGH);

    udp_tx_thread_.Create(udp_tx_thread_fn, this, "udp_tx_thread", 512,
                          System::Thread::HIGH);
  }

  static UartToUDP* self_;

  om_fifo_t udp_tx_fifo_;

  uint8_t can_id_[BSP_CAN_NUM];

  System::Mutex udp_tx_mutex_, udp_tx_sem_;

  System::Thread udp_rx_thread_;

  System::Thread udp_tx_thread_;

  bsp_udp_client_t udp_client_{};
};
}  // namespace Module
