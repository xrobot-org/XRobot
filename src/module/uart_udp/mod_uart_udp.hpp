#include <array>
#include <cstdint>
#include <semaphore.hpp>
#include <string>
#include <thread.hpp>

#include "bsp_time.h"
#include "bsp_uart.h"
#include "bsp_udp_server.h"
#include "module.hpp"
#include "ms.h"
#include "wearlab.hpp"

namespace Module {
class UartToUDP {
 public:
  typedef struct {
    int port;
  } Param;

  UartToUDP(Param& param) {
    for (int i = 0; i < BSP_UART_NUM; i++) {
      udp_rx_sem_[i] = new System::Semaphore(false);
    }

    bsp_udp_server_init(&udp_server_, param.port);

    auto udp_rx_cb = [](void* arg, void* buff, uint32_t size) {
      UartToUDP* udp = static_cast<UartToUDP*>(arg);
      if (size == sizeof(Device::WearLab::UdpData)) {
        Device::WearLab::UdpData* data =
            static_cast<Device::WearLab::UdpData*>(buff);

        memcpy(&udp->udp_rx_[data->area_id], data,
               sizeof(Device::WearLab::UdpData));
        udp->udp_rx_sem_[data->area_id]->Give();
#ifdef MCU_DEBUG_BUILD
        printf("udp pack received.\r\n");
#endif
        return;
      }

#ifdef MCU_DEBUG_BUILD
      printf("udp receive pack error. len:%d\n", size);
#endif
    };

    bsp_udp_register_callback(&udp_server_, BSP_UDP_RX_CPLT_CB, udp_rx_cb,
                              this);

    auto uart_rx_thread_fn = [](UartToUDP* uart_udp) {
      bsp_uart_t uart = static_cast<bsp_uart_t>(uart_udp->num_++);

      uint8_t prefix = 0;
      uint32_t count = 0;

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
#ifdef MCU_DEBUG_BUILD
          printf("uart :%d end data: %d\r\n", uart,
                 uart_udp->uart_rx_[uart].end);
          printf("Recv data error\n\r\n");
#endif
          continue;
        } else {
          uart_udp->header_[uart].raw = uart_udp->uart_rx_[uart].id;
#ifdef MCU_DEBUG_BUILD
          printf("received uart%d pack time:%d count:%d\r\n", uart,
                 bsp_time_get_ms(), count++,
                 uart_udp->header_[uart].data.device_id);
#endif

          uart_udp->udp_tx_[uart].device_id =
              uart_udp->header_[uart].data.device_id;
          uart_udp->udp_tx_[uart].area_id = uart;
          uart_udp->udp_tx_[uart].device_type =
              uart_udp->header_[uart].data.device_type;
          uart_udp->udp_tx_[uart].data_type =
              uart_udp->header_[uart].data.data_type;
          memcpy(uart_udp->udp_tx_[uart].data, uart_udp->uart_rx_[uart].data,
                 8);
          bsp_udp_server_transmit(
              &uart_udp->udp_server_,
              reinterpret_cast<const uint8_t*>(&uart_udp->udp_tx_[uart]),
              sizeof(uart_udp->udp_tx_[uart]));
        }
      }
    };

    auto uart_tx_thread_fn = [](UartToUDP* uart_udp) {
      bsp_uart_t uart = static_cast<bsp_uart_t>(uart_udp->num_);

      while (true) {
        uart_udp->udp_rx_sem_[uart]->Take(UINT32_MAX);
        uart_udp->uart_tx_[uart].id = uart_udp->udp_rx_[uart].device_id;
        memcpy(uart_udp->uart_tx_[uart].data, uart_udp->udp_rx_[uart].data, 8);
        bsp_uart_transmit(uart,
                          reinterpret_cast<uint8_t*>(&uart_udp->uart_tx_[uart]),
                          sizeof(uart_udp->uart_tx_[uart]), true);
      }
    };

    for (int i = 0; i < BSP_UART_NUM; i++) {
      uart_tx_thread_[i].Create(
          uart_tx_thread_fn, this,
          (std::string("uart_to_udp_tx_") + std::to_string(i)).c_str(), 512,
          System::Thread::HIGH);

      uart_rx_thread_[i].Create(
          uart_rx_thread_fn, this,
          (std::string("uart_to_udp_rx_") + std::to_string(i)).c_str(), 512,
          System::Thread::HIGH);
    }

    auto udp_thread_fn = [](UartToUDP* uart_udp) {
      bsp_udp_server_start(&uart_udp->udp_server_);
      while (true) {
        System::Thread::Sleep(UINT32_MAX);
      }
    };

    udp_rx_thread_.Create(udp_thread_fn, this, "udp_tx_thread", 512,
                          System::Thread::HIGH);
  }

  std::array<Device::WearLab::UartData, BSP_UART_NUM> uart_rx_;
  std::array<Device::WearLab::UartData, BSP_UART_NUM> uart_tx_;
  std::array<Device::WearLab::UdpData, BSP_UART_NUM> udp_rx_;
  std::array<Device::WearLab::UdpData, BSP_UART_NUM> udp_tx_;
  std::array<Device::WearLab::CanHeader, BSP_UART_NUM> header_;
  std::array<System::Thread, BSP_UART_NUM> uart_rx_thread_;
  std::array<System::Thread, BSP_UART_NUM> uart_tx_thread_;

  std::array<System::Semaphore*, BSP_UART_NUM> udp_rx_sem_;
  int num_ = 0;
  System::Thread udp_rx_thread_;

  bsp_udp_server_t udp_server_;
};
}  // namespace Module
