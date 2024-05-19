#include "bsp_uart.h"
#include "comp_crc16.hpp"
#include "comp_crc8.hpp"
#include "dev_mt6701.hpp"
#include "module.hpp"

#define FRAME_HEADER_LENGTH 5  // 帧头数据长度
#define CMD_ID_LENGTH 2        // 命令码ID数据长度
#define DATA_LENGTH 30         // 数据段长度
#define FRAME_TAIL_LENGTH 2    // 帧尾数据长度

#define DATA_FRAME_LENGTH                              \
  (FRAME_HEADER_LENGTH + CMD_ID_LENGTH + DATA_LENGTH + \
   FRAME_TAIL_LENGTH)  // 整个数据帧的长度

#define CONTROLLER_CMD_ID 0x0302  // 自定义控制器命令码

typedef struct __attribute__((packed)) {
  struct __attribute__((packed)) {
    uint8_t sof;                 // 起始字节，固定值为0xA5
    uint16_t data_length;        // 数据帧中data的长度
    uint8_t seq;                 // 包序号
    uint8_t crc8;                // 帧头CRC8校验
  } frame_header;                // 帧头
  __packed uint16_t cmd_id;      // 命令码
  __packed uint8_t data[30];     // 自定义控制器的数据帧
  __packed uint16_t frame_tail;  // 帧尾CRC16校验
} Controller;                    // 自定义控制器数据包

namespace Module {
class CustomController {
 public:
  typedef struct {
    Device::MT6701::Param mt6701[6];
  } Param;

  CustomController(Param& param)
      : mt6701_1(param.mt6701[0]),
        mt6701_2(param.mt6701[1]),
        mt6701_3(param.mt6701[2]),
        mt6701_4(param.mt6701[3]),
        mt6701_5(param.mt6701[4]),
        mt6701_6(param.mt6701[5]) {
    auto task_fun = [](CustomController* ctrl) {
      ctrl->txbuff[0] = ctrl->mt6701_1.angle_.Value();
      ctrl->txbuff[1] = ctrl->mt6701_2.angle_.Value();
      ctrl->txbuff[2] = ctrl->mt6701_3.angle_.Value();
      ctrl->txbuff[3] = ctrl->mt6701_4.angle_.Value();
      ctrl->txbuff[4] = ctrl->mt6701_5.angle_.Value();
      ctrl->txbuff[5] = ctrl->mt6701_6.angle_.Value();
      ctrl->DataConcatenation(reinterpret_cast<uint8_t*>(ctrl->txbuff),
                              sizeof(txbuff));
      bsp_uart_transmit(BSP_UART_MCU, reinterpret_cast<uint8_t*>(&ctrl->txdata),
                        DATA_FRAME_LENGTH, false);
    };

    System::Timer::Create(task_fun, this, 40);
  }

  void DataConcatenation(uint8_t* data, uint16_t data_lenth);

  Device::MT6701 mt6701_1;
  Device::MT6701 mt6701_2;
  Device::MT6701 mt6701_3;
  Device::MT6701 mt6701_4;
  Device::MT6701 mt6701_5;
  Device::MT6701 mt6701_6;

  float txbuff[6] = {0};
  Controller txdata;
};
}  // namespace Module
