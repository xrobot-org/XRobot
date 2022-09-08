#include "comp_cmd.hpp"

#include <string.h>

#include "memory.hpp"

using namespace Component;

CMD* CMD::self_;

CMD::CMD() : event_("cmd_event") {
  CMD::self_ = this;

  DECLARE_MESSAGE_FUN(rx_callback) {
    GetMessage(Data, data);
    GetARG(CMD, cmd);

    memcpy((cmd->data_ + data->ctrl_source), data, sizeof(Data));

    if (!cmd->data_[ControlSourceRC].online) {
      cmd->event_.Active(EventLostCtrl);
    }

    if (cmd->ctrl_source_ == ControlSourceRC ||
        (!cmd->data_[cmd->ctrl_source_].online)) {
      memcpy(&(cmd->gimbal_data_.data_), &(cmd->data_[ControlSourceRC].gimbal),
             sizeof(GimbalCMD));
      cmd->gimbal_data_.Publish();
      memcpy(&(cmd->chassis_data_.data_),
             &(cmd->data_[ControlSourceRC].chassis), sizeof(ChassisCMD));
      cmd->chassis_data_.Publish();
    } else if (cmd->ctrl_source_ == ControlSourceAI &&
               cmd->data_[ControlSourceAI].online) {
      memcpy(&(cmd->chassis_data_.data_),
             &(cmd->data_[ControlSourceRC].chassis), sizeof(ChassisCMD));
      cmd->gimbal_data_.Publish();
      memcpy(&(cmd->gimbal_data_.data_), &(cmd->data_[ControlSourceAI].gimbal),
             sizeof(GimbalCMD));
      cmd->chassis_data_.Publish();
    };

    MESSAGE_FUN_PASSED();
  };

  MESSAGE_REGISTER_CALLBACK(this->data_in_, rx_callback, this);
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

    self_->event_.Register(map[i].source, System::Message::Event::EventProgress,
                           cmd_callback, block);
  }
}

void CMD::RegisterController(System::Message::Topic& source) {
  CMD::self_->data_in_.Link(source);
}
