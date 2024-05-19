#pragma once

#include <device.hpp>

#include "bsp_can.h"
#include "comp_ui.hpp"
#include "dev_can.hpp"
#include "dev_motor.hpp"
#include "dev_referee.hpp"

#define DEV_CAP_FB_ID_BASE (0x211)
#define DEV_CAP_CTRL_ID_BASE (0x210)

namespace Device {
class Cap {
 public:
  typedef struct {
    float cap_volt_;
    float output_curr_;
    float output_power_;
    float percentage_;
    float target_power_;
    uint8_t cap_instruct_;
    float cap_volt_max_;
    bool online_;
  } Info;

  typedef struct {
    float power_limit_;
  } Output;

  typedef enum {
    INSTRUCT = 0X600,
    POWER_LIMIT = 0X601,
    OUTPUT_VOLT = 0X602,
    OUTPUT_CUR = 0X603,
    STATE = 0X610,
    INPUT = 0X611,
    OUTPUT = 0X612,
    TP_TIME = 0X613,
  } CanID;
  typedef struct {
    uint16_t ready : 1;
    uint16_t operate : 1;
    uint16_t alarm : 1;
    uint16_t powerswitch : 1;
    uint16_t loadswitch : 1;
    uint16_t const_vlot : 1;
    uint16_t const_cur : 1;
    uint16_t const_power : 1;
    uint16_t retain : 7;
    uint16_t err : 1;
  } ModuleState;
  typedef enum {
    NORMAL,
    INPUT_UNDERVOLT,
    INPUT_OVERVOLT,
    INPUT_OVERCUR,
    INPUT_OVERPOWER,
    PROTECT_OVERTP,
    PROTECT_LOWTP,
    OUTPUT_OVERVOLT,
    OUTPUT_OVERCUR,
    OUTPUT_OVERPOWER,
    ZERO_OVERDRIFT,
    REVERSE_ERRO,
    FAILURE_CONTROL,
    FAILURE_COMMUNICATION,
    FAILURE_ERR,
  } CapState;

  typedef struct {
    bsp_can_t can;
  } Param;

  Cap(Param& param);

  bool Update();

  bool Control(CanID can_id_);

  bool Offline();

  bool InstructUpdata();

  void Decode(Can::Pack& rx);

  float GetPercentage();

  static void DrawUIStatic(Cap* cap);

  static void DrawUIDynamic(Cap* cap);

 private:
  System::Semaphore can_recv_ = System::Semaphore(true);
  System::Semaphore power_limit_update_ = System::Semaphore(true);
  Param param_;

  uint32_t last_online_time_ = 0;

  uint16_t instruct_ = 0;

  Device::BaseMotor::Feedback feedback_;

  System::Queue<Can::Pack> control_feedback_ = System::Queue<Can::Pack>(1);

  System::Thread thread_;

  Message::Topic<Cap::Info> info_tp_;

  Cap::Info info_{};

  Cap::Output out_{};

  Component::UI::String string_{};

  Component::UI::Arc arc_{};
};
}  // namespace Device
