#include "comp_cmd.hpp"

#include <string.h>

#include "memory.hpp"

using namespace Component;

CMD* CMD::self_;

CMD::CMD()
    : event_("cmd_event"),
      data_in_tp_("cmd_data_in"),
      chassis_data_tp_("cmd_chassis"),
      gimbal_data_tp_("cmd_gimbal") {
  CMD::self_ = this;

  auto rx_callback = [](Data& data, CMD* cmd) {
    memcpy(&(cmd->data_[data.ctrl_source]), &data, sizeof(Data));

    if (!cmd->data_[ControlSourceRC].online) {
      cmd->event_.Active(EventLostCtrl);
    }

    if (cmd->ctrl_source_ == ControlSourceRC ||
        (!cmd->data_[cmd->ctrl_source_].online)) {
      cmd->gimbal_data_tp_.Publish(cmd->data_[ControlSourceRC].gimbal);
      cmd->chassis_data_tp_.Publish(cmd->data_[ControlSourceRC].chassis);
    } else if (cmd->ctrl_source_ == ControlSourceAI &&
               cmd->data_[ControlSourceAI].online) {
      cmd->gimbal_data_tp_.Publish(cmd->data_[ControlSourceAI].gimbal);
      cmd->chassis_data_tp_.Publish(cmd->data_[ControlSourceRC].chassis);
    };

    return true;
  };

  this->data_in_tp_.RegisterCallback(rx_callback, this);
}

void CMD::RegisterEvent(void (*callback)(uint32_t event, void* arg), void* arg,
                        const std::vector<Component::CMD::EventMapItem>& map) {
  typedef struct {
    uint32_t target_event;
    void (*callback)(uint32_t event, void* arg);
    void* arg;
  } EventCallbackBlock;

  auto cmd_callback = [](uint32_t event, void* arg) {
    (void)(event);
    EventCallbackBlock* block = static_cast<EventCallbackBlock*>(arg);

    block->callback(block->target_event, block->arg);
  };

  for (size_t i = 0; i < map.size(); i++) {
    EventCallbackBlock* block = static_cast<EventCallbackBlock*>(
        System::Memory::Malloc(sizeof(EventCallbackBlock)));

    block->arg = arg;
    block->callback = callback;
    block->target_event = map[i].target;

    self_->event_.Register(map[i].source, Message::Event::EventProgress,
                           cmd_callback, block);
  }
}

void CMD::RegisterController(Message::Topic<Data>& source) {
  CMD::self_->data_in_tp_.Link(source);
}
