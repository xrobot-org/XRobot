#include "device.hpp"

namespace Device {
class BQ27220 {
 public:
  typedef struct {
    uint16_t capacity_mha;
  } Param;

  typedef struct {
    uint32_t voltage;
    uint32_t temperate;
    uint32_t current;
    uint32_t real_capacity;
    uint16_t percentage;
  } Data;

  BQ27220(Param& param);

  void WriteParam();

  Param param_;

  Data data_ = {};
};
}  // namespace Device
