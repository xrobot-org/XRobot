#include <array>
#include <cstdint>
#include <queue.hpp>
#include <semaphore.hpp>
#include <string>
#include <thread.hpp>

#include "bsp_time.h"
#include "bsp_uart.h"
#include "bsp_udp_server.h"
#include "module.hpp"
#include "ms.h"
#include "om_log.h"
#include "wearlab.hpp"

namespace Module {
class UartToUDP {
 public:
  typedef struct {
    int port;
    bsp_uart_t start_uart;
    bsp_uart_t end_uart;
  } Param;

  UartToUDP(Param& param)
      : param_(param), udp_trans_buff(128), num_lock_(true), udp_tx_sem_(0) {
    for (int i = param.start_uart; i <= param_.end_uart; i++) {
      udp_rx_sem_[i] = new System::Semaphore(false);
      count[i] = 0;
      last_log_time[i] = 0;
    }

    bsp_udp_server_init(&udp_server_, param.port);

    auto udp_rx_cb = [](void* arg, void* buff, uint32_t size) {
      UartToUDP* udp = static_cast<UartToUDP*>(arg);
      if (size == sizeof(Device::WearLab::UdpData)) {
        Device::WearLab::UdpData* data =
            static_cast<Device::WearLab::UdpData*>(buff);

        memcpy(&udp->udp_rx_[data->area_id], data,
               sizeof(Device::WearLab::UdpData));
        udp->udp_rx_sem_[data->area_id]->Post();
        OMLOG_PASS("udp pack received.");
        return;
      }

      OMLOG_ERROR("udp receive pack error. len:%d", size);
    };

    bsp_udp_server_register_callback(&udp_server_, BSP_UDP_RX_CPLT_CB,
                                     udp_rx_cb, this);

    auto uart_rx_thread_fn = [](UartToUDP* uart_udp) {
      uart_udp->num_lock_.Wait(UINT32_MAX);
      bsp_uart_t uart = static_cast<bsp_uart_t>(uart_udp->num_);
      uart_udp->num_++;
      uart_udp->num_lock_.Post();

      uint8_t prefix = 0;

      while (true) {
        do {
          bsp_uart_receive(uart, &prefix, sizeof(prefix), true);
        } while (prefix != 0xa5);

        prefix = 0;

        bsp_uart_receive(uart,
                         reinterpret_cast<uint8_t*>(&uart_udp->uart_rx_[uart]),
                         sizeof(uart_udp->uart_rx_[uart]), true);
        if (uart_udp->uart_rx_[uart].end != 0xe3) {
          bsp_uart_abort_receive(uart);
          OMLOG_ERROR("uart %d receive data error. end:%d", uart,
                      uart_udp->uart_rx_[uart].end);
          continue;
        } else {
          uart_udp->header_[uart].raw = uart_udp->uart_rx_[uart].id;
          uart_udp->udp_tx_[uart].time = bsp_time_get_ms();
          uart_udp->udp_tx_[uart].device_id =
              uart_udp->header_[uart].data.device_id;
          uart_udp->udp_tx_[uart].area_id = uart;
          uart_udp->udp_tx_[uart].device_type =
              uart_udp->header_[uart].data.device_type;
          uart_udp->udp_tx_[uart].data_type =
              uart_udp->header_[uart].data.data_type;
          memcpy(uart_udp->udp_tx_[uart].data, uart_udp->uart_rx_[uart].data,
                 8);
          uart_udp->count[uart]++;

          if (uart_udp->udp_tx_[uart].time - uart_udp->last_log_time[uart] >
              1000) {
            while (uart_udp->udp_tx_[uart].time -
                       uart_udp->last_log_time[uart] >
                   1000) {
              uart_udp->last_log_time[uart] += 1000;
            }
            OMLOG_NOTICE("uart %d get count %d at 1s", uart,
                         uart_udp->count[uart]);
            uart_udp->count[uart] = 0;
          }

          uart_udp->udp_trans_buff.Send(uart_udp->udp_tx_[uart]);
          uart_udp->udp_tx_sem_.Post();
        }
      }
    };

    auto uart_tx_thread_fn = [](UartToUDP* uart_udp) {
      bsp_uart_t uart = static_cast<bsp_uart_t>(uart_udp->num_);

      while (true) {
        uart_udp->udp_rx_sem_[uart]->Wait(UINT32_MAX);
        uart_udp->uart_tx_[uart].id = uart_udp->udp_rx_[uart].device_id;
        memcpy(uart_udp->uart_tx_[uart].data, uart_udp->udp_rx_[uart].data, 8);
        bsp_uart_transmit(uart,
                          reinterpret_cast<uint8_t*>(&uart_udp->uart_tx_[uart]),
                          sizeof(uart_udp->uart_tx_[uart]), true);
      }
    };

    auto udp_rx_thread_fn = [](UartToUDP* uart_udp) {
      bsp_udp_server_start(&uart_udp->udp_server_);
      while (true) {
        System::Thread::Sleep(UINT32_MAX);
      }
    };

    auto udp_tx_thread_fn = [](UartToUDP* uart_udp) {
      std::array<Device::WearLab::UdpData, 128> trans_buff;
      uint32_t buff_num = 0;

      while (true) {
        uart_udp->udp_tx_sem_.Wait(UINT32_MAX);
        buff_num = uart_udp->udp_tx_sem_.Value() + 1;

        uart_udp->udp_trans_buff.Receive(trans_buff[0]);

        for (uint32_t i = 1; i < buff_num; i++) {
          uart_udp->udp_tx_sem_.Wait(UINT32_MAX);
          uart_udp->udp_trans_buff.Receive(trans_buff[i]);
        }

        bsp_udp_server_transmit(&uart_udp->udp_server_,
                                reinterpret_cast<const uint8_t*>(&trans_buff),
                                sizeof(trans_buff[0]) * buff_num);
      }
    };

    for (int i = param_.start_uart; i <= param_.end_uart; i++) {
      uart_tx_thread_[i].Create(
          uart_tx_thread_fn, this,
          (std::string("uart_to_udp_tx_") + std::to_string(i)).c_str(), 512,
          System::Thread::HIGH);
    }

    udp_rx_thread_.Create(udp_rx_thread_fn, this, "udp_rx_thread", 512,
                          System::Thread::HIGH);

    udp_tx_thread_.Create(udp_tx_thread_fn, this, "udp_tx_thread", 512,
                          System::Thread::HIGH);

    System::Thread::Sleep(100);

    for (int i = param_.start_uart; i <= param_.end_uart; i++) {
      uart_rx_thread_[i].Create(
          uart_rx_thread_fn, this,
          (std::string("uart_to_udp_rx_") + std::to_string(i)).c_str(), 512,
          System::Thread::HIGH);
    }
  }

  Param param_;

  std::array<Device::WearLab::UartData, BSP_UART_NUM> uart_rx_{};
  std::array<Device::WearLab::UartData, BSP_UART_NUM> uart_tx_{};
  std::array<Device::WearLab::UdpData, BSP_UART_NUM> udp_rx_{};
  std::array<Device::WearLab::UdpData, BSP_UART_NUM> udp_tx_{};
  std::array<Device::WearLab::CanHeader, BSP_UART_NUM> header_{};
  std::array<System::Thread, BSP_UART_NUM> uart_rx_thread_;
  std::array<System::Thread, BSP_UART_NUM> uart_tx_thread_;

  std::array<System::Semaphore*, BSP_UART_NUM> udp_rx_sem_{};

  System::Queue<Device::WearLab::UdpData> udp_trans_buff;

  System::Semaphore num_lock_;
  int num_ = 0;

  System::Thread udp_rx_thread_;

  System::Thread udp_tx_thread_;

  System::Semaphore udp_tx_sem_;

  bsp_udp_server_t udp_server_{};

  std::array<uint32_t, BSP_UART_NUM> count{};

  std::array<uint32_t, BSP_UART_NUM> last_log_time{};
};
}  // namespace Module
