#include "mod_custom_controller.hpp"

#include "bsp_uart.h"

using namespace Module;

CustomController::CustomController(Param &param) : ma600_(param.config_) {
  for (int i = 0; i < 6; ++i) {
    filters_[i] = Component::LowPassFilter2p(100.0f, 5.f);
  }

  auto task_fun = [](CustomController *ctrl) {
    ctrl->ReadAngles();
    ctrl->DataConcatenation(reinterpret_cast<uint8_t *>(ctrl->txbuff),
                            sizeof(ctrl->txbuff));
    bsp_uart_transmit(BSP_UART_MCU, reinterpret_cast<uint8_t *>(&ctrl->txdata),
                      DATA_FRAME_LENGTH, true);
  };

  System::Timer::Create(task_fun, this, 1);
}

// 读取六个轴的角度值
// void CustomController::ReadAngles() {
//   for (uint8_t i = 0; i < 6; i++) {
//     // 调用 MA600 的 ReadAngle 函数
//     uint16_t angle = ma600_.ReadAngle(i);
//     float raw_angle = static_cast<float>(angle) / 65535.0f * 6.28f;

//     // 对读取的角度数据进行滤波
//     float filtered_angle = filters_[i].Apply(raw_angle);

//     // 存储滤波后的角度值到发送缓冲区
//     txbuff[i] = filtered_angle;
//   }
// }
/* xian fu lv bo*/
void CustomController::ReadAngles() {
  static float prev_angles[6] = {0};    // 用于存储上一次的角度值
  const float max_angle_change = 0.1f;  // 限幅滤波的最大允许变化幅度
  const float jump_threshold = 0.3f;    // 突变检测阈值

  for (uint8_t i = 0; i < 6; i++) {
    // 调用 MA600 的 ReadAngle 函数
    uint16_t angle = ma600_.ReadAngle(i);
    float raw_angle = static_cast<float>(angle) / 65535.0f * 6.28f;

    // 计算角度变化量
    float angle_change = raw_angle - prev_angles[i];

    // 处理 6.28 到 0 的突变
    if (angle_change >
        3.14f) {              // 如果角度变化超过 π，说明发生了 6.28 -> 0 的跳变
      angle_change -= 6.28f;  // 保留跳变
    } else if (angle_change <
               -3.14f) {  // 如果角度变化小于 -π，说明发生了 0 -> 6.28 的跳变
      angle_change += 6.28f;  // 保留跳变
    }

    // 检测 3.14 附近的突变
    if (fabs(angle_change) > jump_threshold) {
      // 如果角度变化量超过阈值，认为是异常跳变，使用上一次的角度值
      raw_angle = prev_angles[i];
    } else {
      // 限幅滤波：限制角度变化的幅度
      if (angle_change > max_angle_change) {
        raw_angle = prev_angles[i] + max_angle_change;
      } else if (angle_change < -max_angle_change) {
        raw_angle = prev_angles[i] - max_angle_change;
      }
    }

    // 更新上一次的角度值
    prev_angles[i] = raw_angle;

    // 对读取的角度数据进行滤波
    float filtered_angle = filters_[i].Apply(raw_angle);

    // 存储滤波后的角度值到发送缓冲区
    txbuff[i] = filtered_angle;
  }
}

Controller txdata;
void CustomController::DataConcatenation(uint8_t *data, uint16_t data_lenth) {
  static uint8_t seq = 0;
  /// 帧头数据
  txdata.frame_header.sof = 0xA5;        // 数据帧起始字节，固定值为 0xA5
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
