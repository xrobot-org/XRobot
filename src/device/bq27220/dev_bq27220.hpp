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

  BQ27220(Param &param);

  static int CMD(BQ27220 *bq27220, int argc, char **argv);

  void WriteParam();

  Param param_;

  System::Term::Command<BQ27220 *> cmd_;

  Data data_ = {};
};
}  // namespace Device
