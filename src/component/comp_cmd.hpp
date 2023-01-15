#pragma once

#include <vector>

#include "component.hpp"

namespace Component {
class CMD {
 public:
  typedef enum { GIMBAL_ABSOLUTE_CTRL, GIMBAL_RELATIVE_CTRL } GimbalCommandMode;

  typedef enum {
    CTRL_SOURCE_RC,
    CTRL_SOURCE_AI,
    CTRL_SOURCE_TERM,
    CTRL_SOURCE_NUM
  } ControlSource;

  typedef enum {
    CMD_OP_CTRL,
    CMD_AUTO_CTRL,
    CMD_TERM_CTRL,
  } Mode;

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

  enum { CMD_EVENT_LOST_CTRL = 0x13212509 };

  typedef struct {
    uint32_t source;
    uint32_t target;
  } EventMapItem;

  CMD(Mode mode = CMD_OP_CTRL);

  static constexpr auto CreateMapItem(uint32_t source, uint32_t target) {
    const EventMapItem ITEM = {source, target};
    return ITEM;
  }

  static void RegisterEvent(
      void (*callback)(uint32_t event, void* arg), void* arg,
      const std::vector<Component::CMD::EventMapItem>& map);

  static void RegisterController(Message::Topic<Data>& source);

  ControlSource ctrl_source_;

  Mode mode_;

  Message::Event event_;

  Data data_[CTRL_SOURCE_NUM];

  Message::Topic<Data> data_in_tp_;
  Message::Topic<ChassisCMD> chassis_data_tp_;
  Message::Topic<GimbalCMD> gimbal_data_tp_;

  static CMD* self_;
};

}  // namespace Component
