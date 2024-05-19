#include "mod_custom_controller.hpp"

#include "bsp_uart.h"

using namespace Module;

Controller txdata;
void CustomController::DataConcatenation(uint8_t *data, uint16_t data_lenth) {
  static uint8_t seq = 0;
  /// 帧头数据
  txdata.frame_header.sof = 0xA5;  // 数据帧起始字节，固定值为 0xA5
  txdata.frame_header.data_length = 30;  // 数据帧中数据段的长度
  txdata.frame_header.seq = seq;         // 包序号
  txdata.frame_header.crc8 = Component::CRC8::Calculate(
      reinterpret_cast<uint8_t *>(&txdata.frame_header),
      FRAME_HEADER_LENGTH - sizeof(uint8_t),
      CRC8_INIT);  // 添加帧头 CRC8 校验位
  /// 命令码ID
  txdata.cmd_id = 0x0302;
  /// 数据段
  memcpy(txdata.data, data, data_lenth);
  /// 帧尾CRC16，整包校验
  txdata.frame_tail = Component::CRC16::Calculate(
      reinterpret_cast<uint8_t *>(&txdata),
      DATA_FRAME_LENGTH - sizeof(uint16_t), CRC16_INIT);
}
using namespace Module;
