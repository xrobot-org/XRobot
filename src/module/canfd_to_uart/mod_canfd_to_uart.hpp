#include "bsp_uart.h"
#include "comp_crc8.hpp"
#include "dev_can.hpp"
#include "module.hpp"

namespace Module {
class FDCanToUart {
 public:
  typedef struct __attribute__((packed)) {
    uint8_t prefix;
    uint8_t id;
    uint32_t index;
    uint8_t data_len : 6;
    uint8_t ext : 1;
    uint8_t fd : 1;
    uint8_t crc8;
  } UartDataHeader;

  FDCanToUart() : uart_received(0), uart_sent(1) {
    self_ = this;

    om_fifo_create(&uart_rx_fifo, new uint8_t[256], 256, sizeof(uint8_t));

    Message::Topic<Device::Can::FDPack>* fd_tp[BSP_CAN_NUM];
    Message::Topic<Device::Can::Pack>* tp[BSP_CAN_NUM];

    for (int i = 0; i < BSP_CAN_NUM; i++) {
      can_id_[i] = i;
      curr_uart_tx_buff[i] = uart_tx_buff[i][0];
    }

    auto uart_tx_cplt_cb = [](void* arg) {
      XB_UNUSED(arg);
      self_->uart_sent.Post();
    };

    auto uart_rx_cplt_cb = [](void* arg) {
      XB_UNUSED(arg);
      bsp_uart_abort_receive(BSP_UART_MCU);
      om_fifo_writes(&self_->uart_rx_fifo, self_->uart_rx_buff,
                     bsp_uart_get_count(BSP_UART_MCU));

      bsp_uart_receive(BSP_UART_MCU, self_->uart_rx_buff,
                       sizeof(self_->uart_rx_buff), false);
      static uint8_t prase_buff[sizeof(UartDataHeader) + 65] = {};
      UartDataHeader* header = reinterpret_cast<UartDataHeader*>(&prase_buff);
      uint32_t len = om_fifo_readable_item_count(&self_->uart_rx_fifo);
      while (len > sizeof(UartDataHeader) + sizeof(uint8_t)) {
        om_fifo_peek(&self_->uart_rx_fifo, header);
        if (header->prefix != 0xa5) {
          om_fifo_pop(&self_->uart_rx_fifo);
          len--;
          continue;
        }

        om_fifo_reads(&self_->uart_rx_fifo, prase_buff, sizeof(UartDataHeader));
        len -= sizeof(UartDataHeader);
        if (!Component::CRC8::Verify(prase_buff, sizeof(UartDataHeader))) {
        };

        if (header->data_len + 1 > len) {
          continue;
        }

        om_fifo_reads(&self_->uart_rx_fifo, prase_buff + sizeof(UartDataHeader),
                      header->data_len + 1);
        len -= header->data_len + 1;
        if (!Component::CRC8::Verify(
                prase_buff, sizeof(UartDataHeader) + header->data_len + 1)) {
          continue;
        }

        static Device::Can::FDPack pack = {};
        pack.info.size = header->data_len;
        pack.info.data = prase_buff + sizeof(UartDataHeader);
        pack.index = header->index;

        if (header->fd) {
          Device::Can::SendFDPack(
              static_cast<bsp_can_t>(header->id),
              header->ext ? CAN_FORMAT_EXT : CAN_FORMAT_STD, header->index,
              prase_buff + sizeof(UartDataHeader), header->data_len);
        } else {
          static Device::Can::Pack pack = {};
          memcpy(pack.data, prase_buff + sizeof(UartDataHeader), 8);
          pack.index = header->index;
          Device::Can::SendPack(static_cast<bsp_can_t>(header->id),
                                header->ext ? CAN_FORMAT_EXT : CAN_FORMAT_STD,
                                pack);
        }
      }
    };

    bsp_uart_register_callback(BSP_UART_MCU, BSP_UART_IDLE_LINE_CB,
                               uart_rx_cplt_cb, this);

    bsp_uart_register_callback(BSP_UART_MCU, BSP_UART_TX_CPLT_CB,
                               uart_tx_cplt_cb, this);

    auto canfd_rx_fun = [](Device::Can::FDPack& pack, uint8_t* can) {
      XB_ASSERT(pack.info.size <= 64);

      if (self_->curr_uart_tx_buff[*can] == self_->uart_tx_buff[*can][0]) {
        self_->curr_uart_tx_buff[*can] = self_->uart_tx_buff[*can][1];
      } else {
        self_->curr_uart_tx_buff[*can] = self_->uart_tx_buff[*can][0];
      }

      uint8_t* buff = self_->curr_uart_tx_buff[*can];
      UartDataHeader* header = reinterpret_cast<UartDataHeader*>(buff);
      buff += sizeof(UartDataHeader);

      header->prefix = 0xa5;
      header->data_len = pack.info.size;
      header->id = *can;
      header->index = pack.index;
      header->fd = true;
      header->crc8 = Component::CRC8::Calculate(
          reinterpret_cast<uint8_t*>(header),
          sizeof(UartDataHeader) - sizeof(uint8_t), CRC8_INIT);
      memcpy(buff, pack.info.data, pack.info.size);
      buff += pack.info.size;
      *buff = Component::CRC8::Calculate(
          reinterpret_cast<uint8_t*>(header),
          sizeof(UartDataHeader) + pack.info.size, CRC8_INIT);
      if (self_->uart_sent.Wait(UINT32_MAX)) {
        bsp_uart_transmit(
            BSP_UART_MCU, reinterpret_cast<uint8_t*>(header),
            sizeof(UartDataHeader) + pack.info.size + sizeof(uint8_t), false);
      }
      return false;
    };

    auto can_rx_fun = [](Device::Can::Pack& pack, uint8_t* can) {
      if (self_->curr_uart_tx_buff[*can] == self_->uart_tx_buff[*can][0]) {
        self_->curr_uart_tx_buff[*can] = self_->uart_tx_buff[*can][1];
      } else {
        self_->curr_uart_tx_buff[*can] = self_->uart_tx_buff[*can][0];
      }

      uint8_t* buff = self_->curr_uart_tx_buff[*can];
      UartDataHeader* header = reinterpret_cast<UartDataHeader*>(buff);
      buff += sizeof(UartDataHeader);

      header->prefix = 0xa5;
      header->data_len = 8;
      header->id = *can;
      header->index = pack.index;
      header->fd = false;
      header->crc8 = Component::CRC8::Calculate(
          reinterpret_cast<uint8_t*>(header),
          sizeof(UartDataHeader) - sizeof(uint8_t), CRC8_INIT);
      memcpy(buff, pack.data, 8);
      buff += 8;
      *buff = Component::CRC8::Calculate(reinterpret_cast<uint8_t*>(header),
                                         sizeof(UartDataHeader) + 8, CRC8_INIT);
      if (self_->uart_sent.Wait(UINT32_MAX)) {
        bsp_uart_transmit(BSP_UART_MCU, buff,
                          sizeof(UartDataHeader) + 8 + sizeof(uint8_t), false);
      }
      return false;
    };

    for (int i = 0; i < BSP_CAN_NUM; i++) {
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

    bsp_uart_receive(BSP_UART_MCU, self_->uart_rx_buff,
                     sizeof(self_->uart_rx_buff), false);
  }

  struct __attribute__((packed)) {
    UartDataHeader header;
    uint8_t buff[65];
  } can_buff_[BSP_CAN_NUM];

  uint8_t* curr_uart_tx_buff[BSP_CAN_NUM];

  uint8_t uart_rx_buff[256] = {};
  uint8_t uart_tx_buff[BSP_CAN_NUM][2][256] = {};

  System::Semaphore uart_received, uart_sent;
  om_fifo_t uart_rx_fifo;

  uint8_t can_id_[BSP_CAN_NUM];

  static FDCanToUart* self_;
};
}  // namespace Module
