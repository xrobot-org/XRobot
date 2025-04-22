#include "device.hpp"

namespace Device {
class SPL06_001 {
 public:
  typedef struct {
    float pressure;
    float temperature;
  } Data;

  typedef struct __attribute__((packed)) {
    int16_t c0 : 12;
    int16_t c1 : 12;
    int32_t c00 : 24;
    int32_t c10 : 24;
    int16_t c01;
    int16_t c11;
    int16_t c20;
    int16_t c21;
    int16_t c30;
  } Cali;

  SPL06_001();

  Cali cali_;

  Data data_;
};
}  // namespace Device
