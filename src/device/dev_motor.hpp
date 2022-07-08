#pragma once

#include <string>

#include "bsp_can.h"
#include "comp_ahrs.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"

/* Motor id */
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
class Motor {
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
    uint8_t num;
    bsp_can_t can;
  } Param;

  typedef struct {
    float rotor_abs_angle;  /* 转子绝对角度 单位：rad */
    float rotational_speed; /* 转速 单位：rpm */
    float torque_current;   /* 转矩电流 单位：A*/
    float temp;             /* 电机温度 单位：℃*/
  } Feedback;

  Motor(const Param& param, const char* name);

  Motor(Motor& motor);

  void Decode(can_rx_item_t& rx);

  bool Update();

  bool AddData();

  bool SendData();

  void Control(float out);

  void Offline();

  float GetLSB();

  Param param_;

  Feedback feedback_;

  uint8_t index_;

  uint32_t mailbox_;

  char name_[20];

  float output_;

  static uint8_t motor_tx_buff_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER]
                               [CAN_DATA_SIZE];

  static uint8_t motor_tx_flag_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER];
  static uint8_t motor_tx_map_[BSP_CAN_NUM][MOTOR_CTRL_ID_NUMBER];

  System::Queue recv_;
};
}  // namespace Device
