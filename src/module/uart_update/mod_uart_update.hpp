#include "bsp_flash.h"
#include "bsp_time.h"
#include "bsp_usb.h"
#include "module.hpp"

namespace Module {
class UartUpdate {
 public:
  typedef struct {
    uint32_t timeout;
    uint32_t board_id;
  } Param;

  typedef struct {
    uint8_t raw[BSP_FLASH_BLOCK_SIZE];
  } Data;

  typedef enum { ACK = 'x', ERROR, GET_ID, WRITE, DATA_SIZE, JUMP_APP } Command;

  typedef struct {
    uint8_t command;
    uint32_t arg;
  } CommandPack;

  Message::Topic<Data> data_topic_;
  Message::Topic<CommandPack> cmd_topic_;
  Message::Remote remote_;

  uint32_t block_offset = 0;

  Param& param_;

  UartUpdate(Param& param)
      : data_topic_("xrobot_update_data"),
        cmd_topic_("xrobot_update_cmd"),
        remote_(2 * BSP_FLASH_BLOCK_SIZE, 2),
        param_(param) {
    bool update = false;

    printf("Uart wait for command...\r\n");

    uint32_t i = bsp_time_get_ms();
    while (i + param.timeout > bsp_time_get_ms()) {
      if (bsp_usb_read_char() == ACK) {
        update = true;
        break;
      }
    }

    if (!update) {
      printf("Uart timeout, skip.\r\n");
      return;
    }

    printf("Get command, start to update by uart.\r\n");

    this->Update();
  }

  void Update();
};
}  // namespace Module
