#pragma once

#include "dev_can.hpp"
#include "device.hpp"

#define DEV_TOF_ID_BASE (0x20c)

namespace Device {
class Tof {
 public:
  typedef enum {
    DEV_TOF_SENSOR_LEFT,
    DEV_TOF_SENSOR_RIGHT,
    DEV_TOF_SENSOR_NUMBER
  } Number;

  typedef struct {
    float dist;
    uint8_t status;
    uint16_t signal_strength;
  } Feedback;

  typedef struct {
    bsp_can_t can;
    uint32_t index;
  } Param;

  Tof(Param& param);

  bool Update();

  void Offline();

  void Decode(Can::Pack& rx);

  Param param_;

  System::Queue<Can::Pack> recv_ = System::Queue<Can::Pack>(1);

  System::Thread thread_;

  Message::Topic<Feedback[DEV_TOF_SENSOR_NUMBER]> fb_tp_ =
      Message::Topic<Feedback[DEV_TOF_SENSOR_NUMBER]>("tof_fb");

  Feedback fb_[DEV_TOF_SENSOR_NUMBER];
};
}  // namespace Device
