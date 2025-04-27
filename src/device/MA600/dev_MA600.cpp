#include "dev_MA600.hpp"

#include "bsp_spi.h"
#include "main.h"  // 包含 HAL 库的头文件

namespace Device {

// 构造函数
MA600::MA600(const std::array<Param, 6>& config) : config_(config) {}

// 读取角度值
uint16_t MA600::ReadAngle(uint8_t axis) {
  if (axis >= config_.size()) {
    return 0;
  }

  const Param& cfg = config_[axis];
  uint8_t tx_data[2] = {0};
  uint8_t rx_data[2] = {0};

  bsp_status_t status =
      bsp_spi_transmit_receive(cfg.cs_pin, tx_data, rx_data, 2, true);
  if (status != BSP_OK) {
    return 0;
  }

  // 组合角度值
  return (rx_data[0] << 8) | rx_data[1];
}

}  // namespace Device
