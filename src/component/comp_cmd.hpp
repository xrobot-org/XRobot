#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <vector>

#include "comp_type.hpp"
#include "list.hpp"
#include "om.hpp"

namespace Component {
class CMD {
 public:
  typedef enum { GimbalAbsoluteCtrl, GimbalRelativeCtrl } GimbalCommandMode;

  typedef enum {
    ControlSourceRC,
    ControlSourceAI,
    ControlSourceNum
  } ControlSource;

  typedef Type::Vector3 ChassisCMD;

  typedef struct {
    Type::Eulr eulr;
    GimbalCommandMode mode;
  } GimbalCMD;

  typedef struct {
    GimbalCMD gimbal;
    ChassisCMD chassis;
    bool online;
    ControlSource ctrl_source;
  } Data;

  enum { EventLostCtrl = 0x13212509 };

  typedef struct {
    uint32_t source;
    uint32_t target;
  } EventMapItem;

  CMD();

  static constexpr auto CreateMapItem(uint32_t source, uint32_t target) {
    const EventMapItem item = {source, target};
    return item;
  }

  static void RegisterEvent(
      void (*callback)(uint32_t event, void* arg), void* arg,
      const std::vector<Component::CMD::EventMapItem>& map);

  static void RegisterController(Message::Topic<Data>& source);

  ControlSource ctrl_source_ = ControlSourceRC;

  Message::Event event_;

  Data data_[ControlSourceNum];

  Message::Topic<Data> data_in_tp_;
  Message::Topic<ChassisCMD> chassis_data_tp_;
  Message::Topic<GimbalCMD> gimbal_data_tp_;

  static CMD* self_;
};

}  // namespace Component
