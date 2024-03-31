#pragma once

#include <component.hpp>
#include <vector>

namespace Component {
class CMD {
 public:
  typedef enum { GIMBAL_ABSOLUTE_CTRL, GIMBAL_RELATIVE_CTRL } GimbalCommandMode;

  typedef enum {
    CTRL_SOURCE_RC,
    CTRL_SOURCE_AI,
    CTRL_SOURCE_TERM,
    CTRL_SOURCE_EXT,
    CTRL_SOURCE_NUM
  } ControlSource;

  typedef enum {
    CMD_OP_CTRL,
    CMD_AUTO_CTRL,
    CMD_TERM_CTRL,
  } Mode;

  typedef Type::Vector3 ChassisCMD;

  typedef struct {
    struct {
      float yaw; /* 偏航角（Yaw angle） */
      float pit; /* 俯仰角（Pitch angle） */
      float rol; /* 翻滚角（Roll angle） */
    } eulr;
    GimbalCommandMode mode;
  } GimbalCMD;

  typedef struct {
    struct {
      float yaw; /* 偏航角（Yaw angle） */
      float pit; /* 俯仰角（Pitch angle） */
      float rol; /* 翻滚角（Roll angle） */
      float x;
      float y;
      float z;
    } extern_channel;

  } ExtCMD;

  typedef struct {
    GimbalCMD gimbal;
    ChassisCMD chassis;
    ExtCMD ext;
    bool online;
    ControlSource ctrl_source;
  } Data;

  enum { CMD_EVENT_LOST_CTRL = 0x13212509 };

  typedef struct {
    uint32_t source;
    uint32_t target;
  } EventMapItem;

  CMD(Mode mode = CMD_OP_CTRL);

  template <typename Type, typename EventType>
  static void RegisterEvent(
      void (*callback)(EventType event, Type arg), Type arg,
      const std::vector<Component::CMD::EventMapItem>& map) {
    typedef struct {
      uint32_t target_event;
      void (*callback)(EventType event, Type arg);
      void* arg;
    } EventCallbackBlock;

    auto cmd_callback = [](uint32_t event, void* arg) {
      XB_UNUSED(event);
      EventCallbackBlock* block = static_cast<EventCallbackBlock*>(arg);

      block->callback(static_cast<EventType>(block->target_event),
                      static_cast<Type>(block->arg));
    };

    std::vector<Component::CMD::EventMapItem>::const_iterator it;

    for (it = map.begin(); it != map.end(); it++) {
      EventCallbackBlock* block = static_cast<EventCallbackBlock*>(
          System::Memory::Malloc(sizeof(EventCallbackBlock)));

      block->arg = arg;
      block->callback = callback;
      block->target_event = it->target;

      self_->event_.Register(it->source, Message::Event::EVENT_PROGRESS,
                             cmd_callback, block);
    }
  }

  static void RegisterController(Message::Topic<Data>& source);

  static void SetCtrlSource(ControlSource source) {
    self_->ctrl_source_ = source;
  }
  static ControlSource GetCtrlSource() { return self_->ctrl_source_; }

  static bool Online() { return self_->online_; }

 private:
  bool online_ = false;
  ControlSource ctrl_source_;

  Mode mode_;

  Message::Event event_;

  std::array<Data, CTRL_SOURCE_NUM> data_{};

  Message::Topic<Data> data_in_tp_;
  Message::Topic<ChassisCMD> chassis_data_tp_;
  Message::Topic<GimbalCMD> gimbal_data_tp_;
  Message::Topic<ExtCMD> ext_data_tp_;
  static CMD* self_;
};

}  // namespace Component
