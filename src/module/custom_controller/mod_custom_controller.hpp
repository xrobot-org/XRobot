#include <array>

#include "bsp_uart.h"
#include "comp_crc16.hpp"
#include "comp_crc8.hpp"
#include "comp_filter.hpp"
#include "dev_MA600.hpp"
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
    std::array<Device::MA600::Param, 6> config_;
  } Param;

  CustomController(Param& param);

  void DataConcatenation(uint8_t* data, uint16_t data_length);

  void ReadAngles();

 private:
  Device::MA600 ma600_;   // MA600 设备对象
  float txbuff[6] = {0};  // 发送缓冲区，存储六个轴的角度值
  Controller txdata{};    // 控制器发送数据结构
  std::array<Component::LowPassFilter2p, 6> filters_;
};

}  // namespace Module
