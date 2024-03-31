#include "comp_cmd.hpp"

using namespace Component;

CMD* CMD::self_;

CMD::CMD(Mode mode)
    : mode_(mode),
      event_("cmd_event"),
      data_in_tp_("cmd_data_in"),
      chassis_data_tp_("cmd_chassis"),
      gimbal_data_tp_("cmd_gimbal"),
      ext_data_tp_("cmd_ext") {
  CMD::self_ = this;

  auto op_ctrl_callback = [](Data& data, CMD* cmd) {
    XB_ASSERT(data.ctrl_source < CTRL_SOURCE_NUM);
    memcpy(&(cmd->data_[data.ctrl_source]), &data, sizeof(Data));

    if (!cmd->data_[CTRL_SOURCE_RC].online && cmd->online_) {
      cmd->event_.Active(CMD_EVENT_LOST_CTRL);
      cmd->online_ = false;
    } else if (cmd->data_[CTRL_SOURCE_RC].online) {
      cmd->online_ = true;
    }

    if (cmd->ctrl_source_ == CTRL_SOURCE_RC ||
        (!cmd->data_[cmd->ctrl_source_].online)) {
      cmd->gimbal_data_tp_.Publish(cmd->data_[CTRL_SOURCE_RC].gimbal);
      cmd->chassis_data_tp_.Publish(cmd->data_[CTRL_SOURCE_RC].chassis);
      cmd->ext_data_tp_.Publish(cmd->data_[CTRL_SOURCE_EXT].ext);
    } else if (cmd->ctrl_source_ == CTRL_SOURCE_AI &&
               cmd->data_[CTRL_SOURCE_AI].online) {
      cmd->gimbal_data_tp_.Publish(cmd->data_[CTRL_SOURCE_AI].gimbal);
      cmd->chassis_data_tp_.Publish(cmd->data_[CTRL_SOURCE_RC].chassis);
      cmd->ext_data_tp_.Publish(cmd->data_[CTRL_SOURCE_AI].ext);
    }

    return true;
  };

  auto auto_ctrl_callback = [](Data& data, CMD* cmd) {
    memcpy(&(cmd->data_[data.ctrl_source]), &data, sizeof(Data));

    if (!cmd->data_[CTRL_SOURCE_RC].online && cmd->online_) {
      cmd->event_.Active(CMD_EVENT_LOST_CTRL);
      cmd->online_ = false;
    } else if (cmd->data_[CTRL_SOURCE_RC].online) {
      cmd->online_ = true;
    }

    if (cmd->ctrl_source_ == CTRL_SOURCE_RC ||
        (!cmd->data_[cmd->ctrl_source_].online)) {
      cmd->gimbal_data_tp_.Publish(cmd->data_[CTRL_SOURCE_RC].gimbal);
      cmd->chassis_data_tp_.Publish(cmd->data_[CTRL_SOURCE_RC].chassis);
    } else if (cmd->ctrl_source_ == CTRL_SOURCE_AI &&
               cmd->data_[CTRL_SOURCE_AI].online) {
      cmd->gimbal_data_tp_.Publish(cmd->data_[CTRL_SOURCE_AI].gimbal);
      cmd->chassis_data_tp_.Publish(cmd->data_[CTRL_SOURCE_AI].chassis);
    }

    return true;
  };

  auto term_ctrl_callback = [](Data& data, CMD* cmd) {
    memcpy(&(cmd->data_[CTRL_SOURCE_TERM]), &data, sizeof(Data));

    cmd->gimbal_data_tp_.Publish(cmd->data_[CTRL_SOURCE_TERM].gimbal);
    cmd->chassis_data_tp_.Publish(cmd->data_[CTRL_SOURCE_TERM].chassis);

    return true;
  };

  switch (this->mode_) {
    case CMD_OP_CTRL:
      this->ctrl_source_ = CTRL_SOURCE_RC;
      this->data_in_tp_.RegisterCallback(op_ctrl_callback, this);
      break;
    case CMD_AUTO_CTRL:
      this->ctrl_source_ = CTRL_SOURCE_AI;
      this->data_in_tp_.RegisterCallback(auto_ctrl_callback, this);
      break;
    case CMD_TERM_CTRL:
      this->ctrl_source_ = CTRL_SOURCE_TERM;
      this->data_in_tp_.RegisterCallback(term_ctrl_callback, this);
      break;
  }
}

void CMD::RegisterController(Message::Topic<Data>& source) {
  CMD::self_->data_in_tp_.Link(source);
}
