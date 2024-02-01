#include "device.hpp"

namespace Device {
class BQ27220 {
 public:
  typedef struct {
    uint16_t capacity_mha;
  } Param;

  typedef struct {
    float voltage;
    float temperate;
    float current;
    int16_t real_capacity;
    uint8_t percentage;
  } Data;

  BQ27220(Param& param);

  void WriteParam();

  Param param_;

  Data data_ = {};
};
}  // namespace Device
