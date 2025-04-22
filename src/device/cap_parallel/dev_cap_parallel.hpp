#pragma once

#include "bsp_can.h"
#include "comp_cmd.hpp"
#include "comp_ui.hpp"
#include "dev_can.hpp"
#include "dev_motor.hpp"
#include "dev_referee.hpp"
#include "device.hpp"

namespace Device {
class CapParallel {
 public:
  typedef struct {
    float cap_volt_;
    float cap_cur_;
    float output_power_;
    float percentage_;
    float target_power_;
    uint8_t cap_instruct_;
    bool online_;
  } Info;
  typedef enum {
    OPEN,
    OFF,
  } CapMode;
  typedef enum {
    SET_MODE_OPEN,
    SET_MODE_OFF,
  } CapEvent;
  typedef struct {
    float power_limit_;
  } Output;
  typedef struct {
    float power_limit_;
  } Input;
  typedef enum {
    POWERBUF = 0x2E,
    CAPCONTROL = 0x2f,
    CAPFEEDBACK = 0x30,
  } CanId;
  typedef struct {
    uint16_t OverVlot;
    uint16_t OverCurrent;
    uint16_t UnderVlot;
    uint16_t RefUnderVlot;
    uint16_t CanNull;
    uint16_t Any;
  } CapState;
  typedef struct {
    bsp_can_t can;
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;
  } Param;

  CapParallel(Param& param);

  bool Update();

  bool Control(CanId can_id_);

  bool Offline();

  void SetMode(CapParallel::CapMode mode);

  bool InstructUpdata();

  void GetCapState();

  void Decode(Can::Pack& rx);

  float GetPercentage();

  static void DrawUIStatic(CapParallel* cap);

  static void DrawUIDynamic(CapParallel* cap);

 private:
  Param param_;

  uint32_t last_online_time_ = 0.0f;

  int cap_state_;

  CapState capstate_;

  CapMode mode_ = OFF;

  Device::Referee::Data raw_ref_;

  Device::BaseMotor::Feedback feedback_;

  System::Queue<Can::Pack> control_feedback_ = System::Queue<Can::Pack>(1);

  System::Thread thread_;

  Message::Topic<CapParallel::Info> info_tp_;

  System::Semaphore ctrl_lock_;

  CapParallel::Info info_{};

  CapParallel::Input input_{};

  CapParallel::Output out_{};

  Component::UI::String string_{};

  Component::UI::Arc arc_{};
};
}  // namespace Device
