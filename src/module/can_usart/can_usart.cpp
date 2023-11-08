#include "can_usart.hpp"

#include "bsp_can.h"
#include "bsp_uart.h"

using namespace Module;

CantoUsart::CantoUsart()
    : uart_recv_buff_addr_(&uart_recv_buff_1_),
      tx_cplt_(true),
      tx_ready_(false) {
  auto rx_callback_fn = [](void *arg) {
    CantoUsart *can_uart = static_cast<CantoUsart *>(arg);

    bsp_uart_abort_receive(BSP_UART_MCU);

    uint8_t *data = &((*can_uart->uart_recv_buff_addr_)[0]);

    uint32_t len = bsp_uart_get_count(BSP_UART_MCU);
    int32_t index = 0;

    if (can_uart->uart_recv_buff_addr_ == &can_uart->uart_recv_buff_1_) {
      can_uart->uart_recv_buff_addr_ = &can_uart->uart_recv_buff_2_;
    } else {
      can_uart->uart_recv_buff_addr_ = &can_uart->uart_recv_buff_1_;
    }

    bsp_uart_receive(BSP_UART_MCU, &((*can_uart->uart_recv_buff_addr_)[0]),
                     sizeof(can_uart->uart_recv_buff_1_), false);

    while (index <=
           static_cast<int32_t>(len) - static_cast<int32_t>(sizeof(UartData))) {
      auto pack = reinterpret_cast<UartData *>(data + index);
      if (pack->start_frame != START || pack->end_frame != END) {
        index++;
        continue;
      } else {
        bsp_can_trans_packet(BSP_CAN_1,
                             static_cast<bsp_can_format_t>(pack->type),
                             pack->id, pack->data);
        index += sizeof(UartData);
      }
    }
  };

  bsp_uart_register_callback(BSP_UART_MCU, BSP_UART_IDLE_LINE_CB,
                             rx_callback_fn, this);

  bsp_uart_receive(BSP_UART_MCU, &uart_recv_buff_1_[0],
                   sizeof(uart_recv_buff_1_), false);

  auto tx_cplt_cb = [](void *arg) {
    CantoUsart *can_uart = static_cast<CantoUsart *>(arg);

    can_uart->tx_cplt_.Post();
  };

  bsp_uart_register_callback(BSP_UART_MCU, BSP_UART_TX_CPLT_CB, tx_cplt_cb,
                             this);

  auto rx_callback = [](Device::Can::Pack &rx, CantoUsart *can_uart) {
    can_uart->uart_trans_buff_.start_frame = START;
    can_uart->uart_trans_buff_.id = rx.index;
    can_uart->uart_trans_buff_.type = CAN_FORMAT_STD;
    memcpy(&can_uart->uart_trans_buff_.data, rx.data, sizeof(rx.data));
    can_uart->uart_trans_buff_.end_frame = END;
    can_uart->tx_ready_.Post();
    return true;
  };

  Message::Topic<Device::Can::Pack> cap_tp("can_to_usart");
  cap_tp.RegisterCallback(rx_callback, this);

  Device::Can::Subscribe(cap_tp, BSP_CAN_1, 0, UINT32_MAX);

  auto uart_send_fn = [](CantoUsart *can_uart) {
    while (1) {
      can_uart->tx_ready_.Wait(UINT32_MAX);
      can_uart->tx_cplt_.Wait(UINT32_MAX);
      bsp_uart_transmit(
          BSP_UART_MCU,
          reinterpret_cast<uint8_t *>(&can_uart->uart_trans_buff_),
          sizeof(UartData), false);
    }
  };

  System::Timer::Create(uart_send_fn, this, 0);
}
