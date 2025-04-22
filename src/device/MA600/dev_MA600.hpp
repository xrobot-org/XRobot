#ifndef DEV_MA600_HPP
#define DEV_MA600_HPP

#include <array>
#include <cstdint>

#include "bsp_spi.h"

namespace Device {

class MA600 {
 public:
  typedef struct {
    bsp_spi_t cs_pin;  // 片选引脚
    uint32_t timeout;  // SPI 超时时间
  } Param;

  MA600(const std::array<Param, 6>& config);

  uint16_t ReadAngle(uint8_t axis);

  Param param_;

 private:
  std::array<Param, 6> config_;
};

}  // namespace Device

#endif
