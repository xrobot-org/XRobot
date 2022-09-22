#pragma once

#include "bsp_can.h"
#include "comp_type.hpp"
#include "dev.hpp"
#include "dev_can.hpp"

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

  void Decode(CAN::Pack& rx);

  Param param_;

  System::Queue recv_;

  System::Thread thread_;

  DECLARE_PUBER(fb_, Feedback[DEV_TOF_SENSOR_NUMBER], "tof_fb", true);
};
}  // namespace Device
