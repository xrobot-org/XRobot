#pragma once

#include <device.hpp>

#include "bsp_can.h"
#include "dev_can.hpp"
#include "dev_motor.hpp"

/* RMMotor id */
/* id     feedback id     control id */
/* 1-4    0x205 to 0x208  0x1ff */
/* 5-6    0x209 to 0x20B  0x2ff */
#define GM6020_FB_ID_BASE (0x205)
#define GM6020_FB_ID_EXTAND (0x209)
#define GM6020_CTRL_ID_BASE (0x1ff)
#define GM6020_CTRL_ID_EXTAND (0x2ff)

/* id     feedback id		  control id */
/* 1-4		0x201 to 0x204  0x200 */
/* 5-6		0x205 to 0x208  0x1ff */
#define M3508_M2006_FB_ID_BASE (0x201)
#define M3508_M2006_FB_ID_EXTAND (0x205)
#define M3508_M2006_CTRL_ID_BASE (0x200)
#define M3508_M2006_CTRL_ID_EXTAND (0x1ff)
#define M3508_M2006_ID_SETTING_ID (0x700)

#define MOTOR_CTRL_ID_NUMBER (3)

namespace Device {
class RMMotor : public BaseMotor {
 public:
  typedef enum {
    MOTOR_NONE = 0,
    MOTOR_M2006,
    MOTOR_M3508,
    MOTOR_GM6020,
  } Model;

  typedef struct {
    uint32_t id_feedback;
    uint32_t id_control;
    Model model;
    bsp_can_t can;
    bool reverse;
  } Param;

  RMMotor(const Param& param, const char* name);

  RMMotor(RMMotor& motor);

  void Decode(Can::Pack& rx);

  bool Update();

  bool SendData();

  void Control(float output);

  void Offline();

  float GetLSB();

  void Relax();

 private:
  Param param_;

  uint8_t num_;

  uint8_t index_;

  float output_;

  static uint8_t motor_tx_buff_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER][8];

  static uint8_t motor_tx_flag_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER];

  static uint8_t motor_tx_map_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER];

  System::Queue<Can::Pack> recv_ = System::Queue<Can::Pack>(1);
};
}  // namespace Device
