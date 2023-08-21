#pragma once

#include <device.hpp>

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
    EULR_DATA_ID = 0x03,
    QUAT_DATA_ID = 0x04
  } ID;

  IMU(Param& param);

  void Update();

  bool Offline();

  bool Decode(Device::Can::Pack& rx);

 private:
  Param param_;

  uint32_t last_online_time_ = 0;

  bool online_ = false;

  Message::Topic<Component::Type::Vector3> accl_tp_;
  Message::Topic<Component::Type::Vector3> gyro_tp_;
  Message::Topic<Component::Type::Eulr> eulr_tp_;

  Component::Type::Vector3 accl_{};
  Component::Type::Vector3 gyro_{};
  Component::Type::Eulr eulr_{};

  System::Queue<Device::Can::Pack> recv_ = System::Queue<Can::Pack>(4);

  System::Thread thread_;
};

}  // namespace Device
