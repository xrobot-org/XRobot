#include <device.hpp>

#include "comp_cmd.hpp"

namespace Device {
class TerminalController {
 public:
  typedef enum {
    START_CTRL = 25,
    STOP_CTRL,
  } Event;

  TerminalController();

  static int ControlCMD(TerminalController* ctrl, int argc, char* argv[]);

  Message::Event event_;

  Message::Topic<Component::CMD::Data> cmd_tp_;

  Component::CMD::Data cmd_data_;

  System::Term::Command<TerminalController*> cmd_;

  Message::Topic<float> yaw_tp_ = Message::Topic<float>("chassis_yaw");
};
}  // namespace Device
