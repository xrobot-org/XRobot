#include "bsp_can.h"
#include "device.hpp"

namespace Device {
class MicroSwitch {
 public:
  typedef struct {
    bsp_can_t can;
    uint32_t id;
  } Param;

  typedef enum { OFF, ON } Status;

  typedef struct {
    uint8_t id;
    Status status;
  } Data;

  MicroSwitch(Param& param);

  static void Subscriber(Data& data, uint8_t switch_id);

  static Message::Topic<Data>* topic_;

  static bool inited_;

  Param& param_;

  Data data_{};
};
}  // namespace Device
