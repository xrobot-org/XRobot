#include "dev_controller.hpp"

using namespace Device;

TerminalController::TerminalController()
    : event_(Message::Event::FindEvent("cmd_event")),
      cmd_tp_("cmd_rc"),
      cmd_(this, this->ControlCMD, "control", System::Term::DevDir()) {
  Component::CMD::RegisterController(this->cmd_tp_);
}

void TerminalController::ControlCMD(TerminalController* ctrl, int argc,
                                    char* argv[]) {
  if (argc == 1) {
    ms_printf("[w/a/s/d] [speed 0-100] [time ms] 向某个方向运动");
    ms_enter();
    ms_printf("[start/stop]             运行/停止");
    ms_enter();
  } else if (argc == 2) {
    if (!strcmp(argv[1], "start")) {
      ctrl->event_.Active(Start);
    } else if (!strcmp(argv[1], "stop")) {
      ctrl->event_.Active(Stop);
    } else {
      ms_printf("参数错误");
      ms_enter();
      return;
    }
  } else if (argc == 4) {
    float x = 0.0f, y = 0.0f;
    switch (argv[1][0]) {
      case 'w':
        y = 1.0f;
        break;
      case 'a':
        x = -1.0f;
        break;
      case 's':
        y = -1.0f;
        break;
      case 'd':
        x = 1.0f;
        break;
      default:
        ms_printf("方向错误");
        ms_enter();
        return;
    }

    float speed = std::stoi(argv[2]);
    float time = std::stoi(argv[3]);

    ctrl->cmd_data_.chassis.x = x * speed / 100.0f;
    ctrl->cmd_data_.chassis.y = y * speed / 100.0f;

    ctrl->cmd_data_.ctrl_source = Component::CMD::ControlSourceTerm;

    ctrl->cmd_tp_.Publish(ctrl->cmd_data_);

    System::Thread::Sleep(time);

    ctrl->cmd_data_.chassis.x = 0.0f;
    ctrl->cmd_data_.chassis.y = 0.0f;

    ctrl->cmd_tp_.Publish(ctrl->cmd_data_);
  } else {
    ms_printf("参数错误");
    ms_enter();
  }
}
