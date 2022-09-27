#pragma once

#include "comp_type.hpp"
#include "dev.hpp"
#include "dev_can.hpp"

namespace Device {
class IMU {
 public:
  typedef struct {
    const char* tp_name_prefix;
    bsp_can_t can;
    uint32_t index;
  } Param;

  typedef enum {
    IMU_DEVICE_ID = 0x01,
    ACCL_DATA_ID = 0x01,
    GYRO_DATA_ID = 0x02,
    EULR_DATA_ID = 0x03
  } ID;

  IMU(Param& param);

  void Update();

  bool Offline();

  bool Decode(CAN::Pack& rx);

  Param param_;

  uint32_t last_online_time_;

  bool online_;

  System::Message::Publisher<Component::Type::Vector3> accl_;
  System::Message::Publisher<Component::Type::Vector3> gyro_;
  System::Message::Publisher<Component::Type::Eulr> eulr_;

  System::Queue recv_ = System::Queue(sizeof(CAN::Pack), 4);

  System::Thread thread_;
};
}  // namespace Device
