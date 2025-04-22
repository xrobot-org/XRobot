#include "mod_uart_update.hpp"

#include "bsp_def.h"
#include "bsp_sys.h"
#include "bsp_uart.h"
#include "bsp_usb.h"
#include "om_core.h"

using namespace Module;

static uint8_t uart_buff[BSP_FLASH_BLOCK_SIZE * 2];

void UartUpdate::Update() {
  remote_.AddTopic(data_topic_);
  remote_.AddTopic(cmd_topic_);

  auto data_tp_cb = [](Data& data, UartUpdate* uart_update) {
    XB_UNUSED(uart_update);
    static CommandPack buff;
    buff.command = ACK;
    buff.arg = uart_update->block_offset;
    bsp_usb_transmit(reinterpret_cast<uint8_t*>(&buff), sizeof(buff));
    bsp_flash_wirte((void*)(BSP_FLASH_APP_ADDR + uart_update->block_offset),
                    BSP_FLASH_BLOCK_SIZE, data.raw);
    return true;
  };

  auto cmd_tp_cb = [](CommandPack& data, UartUpdate* uart_update) {
    XB_UNUSED(uart_update);

    switch (data.command) {
      case ACK:
        break;
      case ERROR:
        bsp_sys_reset();
        break;
      case GET_ID:
        data.arg = uart_update->param_.board_id;
        break;
      case WRITE:
        uart_update->block_offset = data.arg;
        bsp_usb_read(uart_buff, BSP_FLASH_BLOCK_SIZE);
        if (!uart_update->remote_.PraseData(uart_buff, BSP_FLASH_BLOCK_SIZE)) {
          data.command = ERROR;
        }
        break;
      case DATA_SIZE:
        data.arg = BSP_FLASH_BLOCK_SIZE;
        break;
      case JUMP_APP:
        bsp_sys_jump_app();
        break;
    }

    bsp_usb_transmit(reinterpret_cast<uint8_t*>(&data), sizeof(data));

    return true;
  };

  bsp_uart_abort_receive(BSP_UART_MCU);

  data_topic_.RegisterCallback(data_tp_cb, this);
  cmd_topic_.RegisterCallback(cmd_tp_cb, this);

  while (1) {
    bsp_usb_read(uart_buff, sizeof(CommandPack));
    remote_.PraseData(uart_buff, sizeof(CommandPack));
  }
}
