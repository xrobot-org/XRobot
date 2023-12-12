#include "bsp_can.h"

#include <poll.h>
#include <pthread.h>

#include <array>

#include "bsp_def.h"
#include "bsp_uart.h"

#define CRC8_INIT 0Xff

typedef struct __attribute__((packed)) {
  uint8_t prefix;
  uint8_t id;
  uint32_t index;
  uint8_t data_len : 6;
  uint8_t ext : 1;
  uint8_t fd : 1;
  uint8_t crc8;
} UartDataHeader;

enum { BSP_CAN_UART1, BSP_CAN_UART2, BSP_CAN_UART_NUM };

typedef struct {
  void (*fn)(bsp_can_t can, uint32_t id, uint8_t *data, void *arg);
  void *arg;
} can_callback_t;

static const std::array<uint8_t, 256> CRC8_TAB = {
    0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20,
    0xa3, 0xfd, 0x1f, 0x41, 0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
    0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc, 0x23, 0x7d, 0x9f, 0xc1,
    0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
    0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e,
    0x1d, 0x43, 0xa1, 0xff, 0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
    0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07, 0xdb, 0x85, 0x67, 0x39,
    0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
    0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45,
    0xc6, 0x98, 0x7a, 0x24, 0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
    0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9, 0x8c, 0xd2, 0x30, 0x6e,
    0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
    0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31,
    0xb2, 0xec, 0x0e, 0x50, 0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
    0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee, 0x32, 0x6c, 0x8e, 0xd0,
    0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
    0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea,
    0x69, 0x37, 0xd5, 0x8b, 0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
    0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16, 0xe9, 0xb7, 0x55, 0x0b,
    0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
    0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54,
    0xd7, 0x89, 0x6b, 0x35};

uint8_t calculate(const uint8_t *buf, size_t len, uint8_t crc) {
  /* loop over the buffer data */
  while (len-- > 0) {
    crc = CRC8_TAB[(crc ^ *buf++) & 0xff];
  }
  return crc;
}

bool verify(const uint8_t *buf, size_t len) {
  if (len < 2) {
    return false;
  }

  uint8_t expected = calculate(buf, len - sizeof(uint8_t), CRC8_INIT);
  return expected == buf[len - sizeof(uint8_t)];
}

static can_callback_t callback_list[BSP_CAN_NUM][BSP_CAN_CB_NUM];

static uint8_t uart_rx_buff[64][BSP_CAN_UART_NUM];

static uint8_t uart_tx_buff[128][BSP_CAN_UART_NUM];

static pthread_mutex_t tx_mutex[BSP_CAN_UART_NUM] = {PTHREAD_MUTEX_INITIALIZER,
                                                     PTHREAD_MUTEX_INITIALIZER};

inline bsp_can_t bsp_can_get(bsp_uart_t uart, uint8_t id) {
  return static_cast<bsp_can_t>(uart * 2 + id);
}

inline bsp_uart_t bsp_can_get_uart(bsp_can_t can) {
  if (can <= BSP_CAN_2) {
    return BSP_UART_1;
  } else {
    return BSP_UART_2;
  }
}

inline uint8_t bsp_can_get_id(bsp_can_t can) { return can % 2; }

void bsp_can_init(void) {
  auto uart_rx_thread_fn = [](void *arg) {
    bsp_uart_t uart = *static_cast<bsp_uart_t *>(arg);

    while (true) {
      auto index = uart_rx_buff[uart];
      do {
        bsp_uart_receive(uart, uart_rx_buff[uart], sizeof(uint8_t), true);
      } while (*uart_rx_buff[uart] != 0xa5);

      index++;

      bsp_uart_receive(uart, index, sizeof(UartDataHeader) - 1, true);
      index += sizeof(UartDataHeader) - 1;

      if (!verify(uart_rx_buff[uart], sizeof(UartDataHeader))) {
        continue;
      }

      auto header = reinterpret_cast<UartDataHeader *>(uart_rx_buff[uart]);

      bsp_uart_receive(uart, index, header->data_len + 1, true);

      if (!verify(uart_rx_buff[uart],
                  sizeof(UartDataHeader) + header->data_len + 1)) {
        continue;
      }

      if (header->fd) {
        if (callback_list[bsp_can_get(uart, header->id)][CANFD_RX_MSG_CALLBACK]
                .fn) {
          bsp_canfd_data_t data = {.size = header->data_len, .data = index};
          callback_list[bsp_can_get(uart, header->id)][CANFD_RX_MSG_CALLBACK]
              .fn(bsp_can_get(uart, header->id), header->index,
                  reinterpret_cast<uint8_t *>(&data),
                  callback_list[bsp_can_get(uart, header->id)]
                               [CANFD_RX_MSG_CALLBACK]
                                   .arg);
        } else {
          if (callback_list[bsp_can_get(uart, header->id)][CAN_RX_MSG_CALLBACK]
                  .fn) {
            callback_list[bsp_can_get(uart, header->id)][CAN_RX_MSG_CALLBACK]
                .fn(bsp_can_get(uart, header->id), header->index, index,
                    callback_list[bsp_can_get(uart, header->id)]
                                 [CAN_RX_MSG_CALLBACK]
                                     .arg);
          }
        }
      }
    }

    return static_cast<void *>(0);
  };

  static bsp_uart_t uart[BSP_CAN_UART_NUM];
  static pthread_t thread[BSP_CAN_UART_NUM];

  for (int i = 0; i < BSP_CAN_UART_NUM; i++) {
    uart[i] = static_cast<bsp_uart_t>(i);
    pthread_create(&thread[i], NULL, uart_rx_thread_fn, &uart[i]);
  }
}

bsp_status_t bsp_can_register_callback(
    bsp_can_t can, bsp_can_callback_t type,
    void (*callback)(bsp_can_t can, uint32_t id, uint8_t *data, void *arg),
    void *callback_arg) {
  XB_ASSERT(callback);
  XB_ASSERT(type != BSP_CAN_CB_NUM);

  callback_list[can][type].fn = callback;
  callback_list[can][type].arg = callback_arg;
  return BSP_OK;
};

bsp_status_t bsp_can_trans_packet(bsp_can_t can, bsp_can_format_t format,
                                  uint32_t id, uint8_t *data) {
  auto uart = bsp_can_get_uart(can);

  pthread_mutex_lock(&tx_mutex[uart]);
  auto header = reinterpret_cast<UartDataHeader *>(uart_tx_buff[uart]);
  header->prefix = 0xa5;
  header->id = bsp_can_get_id(can);
  header->data_len = 8;
  header->fd = false;
  header->ext = (format == CAN_FORMAT_EXT) ? 1 : 0;
  header->index = id;
  header->crc8 = calculate(reinterpret_cast<uint8_t *>(header),
                           sizeof(UartDataHeader) - 1, CRC8_INIT);
  memcpy(uart_tx_buff[uart] + sizeof(UartDataHeader), data, 8);
  *(uart_tx_buff[uart] + sizeof(UartDataHeader) + 8) =
      calculate(uart_tx_buff[uart], sizeof(UartDataHeader) + 8, CRC8_INIT);
  bsp_uart_transmit(uart, uart_tx_buff[uart], sizeof(UartDataHeader) + 9, true);
  pthread_mutex_unlock(&tx_mutex[uart]);
  return BSP_OK;
}

bsp_status_t bsp_canfd_trans_packet(bsp_can_t can, bsp_can_format_t format,
                                    uint32_t id, uint8_t *data, size_t size) {
  if (format == CAN_FORMAT_STD) {
    XB_ASSERT(false);
  }

  if (size > 64) {
    XB_ASSERT(false);
  }

  auto uart = bsp_can_get_uart(can);

  pthread_mutex_lock(&tx_mutex[uart]);
  auto header = reinterpret_cast<UartDataHeader *>(uart_tx_buff[uart]);
  header->prefix = 0xa5;
  header->id = bsp_can_get_id(can);
  header->data_len = size;
  header->fd = true;
  header->ext = (format == CAN_FORMAT_EXT) ? 1 : 0;
  header->index = id;
  header->crc8 = calculate(reinterpret_cast<uint8_t *>(header),
                           sizeof(UartDataHeader) - 1, CRC8_INIT);
  memcpy(uart_tx_buff[uart] + sizeof(UartDataHeader), data, size);
  *(uart_tx_buff[uart] + sizeof(UartDataHeader) + size) =
      calculate(uart_tx_buff[uart], sizeof(UartDataHeader) + size, CRC8_INIT);
  bsp_uart_transmit(uart, uart_tx_buff[uart], sizeof(UartDataHeader) + size + 1,
                    true);
  pthread_mutex_unlock(&tx_mutex[uart]);
  return BSP_OK;
}
