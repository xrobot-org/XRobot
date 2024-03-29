#include "dev_controller.hpp"

#include <webots/keyboard.h>

using namespace Device;

TerminalController::TerminalController()
    : event_(Message::Event::FindEvent("cmd_event")),
      cmd_tp_("cmd_rc"),
      cmd_(this, this->ControlCMD, "control", System::Term::BinDir()) {
  Component::CMD::RegisterController(this->cmd_tp_);
}

int TerminalController::ControlCMD(TerminalController* ctrl, int argc,
                                   char** argv) {
  if (argc == 1) {
    printf("[w/a/s/d] [speed 0-100] [time ms] 向某个方向运动\r\n");
    printf("[start/stop]             运行/停止\r\n");
    printf("[keyboard]                使用键鼠控制\r\n");
  } else if (argc == 2) {
    if (!strcmp(argv[1], "start")) {
      ctrl->event_.Active(START_CTRL);
    } else if (!strcmp(argv[1], "stop")) {
      ctrl->event_.Active(STOP_CTRL);
    } else if (!strcmp(argv[1], "keyboard")) {
      printf(
          "Start keyboard control. press w/a/s/d to move, q/e to turn, and r "
          "to exit.\r\n");

      wb_keyboard_enable(30);

      float x = 0.0f, y = 0.0f, z = 0.0f;

      while (1) {
        switch (wb_keyboard_get_key()) {
          case 'W':
            x = 0.0f;
            y = 0.8f;
            z = 0.0f;
            break;
          case 'A':
            x = -0.8f;
            y = 0.0f;
            z = 0.0f;
            break;
          case 'S':
            x = 0.0f;
            y = -0.8f;
            z = 0.0f;
            break;
          case 'D':
            x = 0.8f;
            y = -0.0f;
            z = 0.0f;
            break;
          case 'Q':
            x = 0.0f;
            y = 0.0f;
            z = -0.4f;
            break;
          case 'E':
            x = 0.0f;
            y = -0.0f;
            z = 0.4f;
            break;
          case 'R':
            wb_keyboard_disable();

            printf("exit.\r\n");

            return 0;
          default:
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
        }

        ctrl->cmd_data_.chassis.x = x;
        ctrl->cmd_data_.chassis.y = y;
        ctrl->cmd_data_.chassis.z = z;

        ctrl->cmd_data_.ctrl_source = Component::CMD::CTRL_SOURCE_TERM;

        ctrl->cmd_tp_.Publish(ctrl->cmd_data_);

        System::Thread::Sleep(30);

        ctrl->cmd_data_.chassis.x = 0.0f;
        ctrl->cmd_data_.chassis.y = 0.0f;
        ctrl->cmd_data_.chassis.z = 0.0f;
        ctrl->cmd_tp_.Publish(ctrl->cmd_data_);
      }

    } else {
      printf("参数错误\r\n");
      return -1;
    }
  } else if (argc == 4) {
    float x = 0.0f, y = 0.0f;
    switch (argv[1][0]) {
      case 'w':
        y = 0.8f;
        break;
      case 'a':
        x = -0.8f;
        break;
      case 's':
        y = -0.8f;
        break;
      case 'd':
        x = 0.8f;
        break;
      default:
        printf("方向错误\r\n");
        return -1;
    }

    int speed = std::stoi(argv[2]);
    int time = std::stoi(argv[3]);

    ctrl->cmd_data_.chassis.x = x * static_cast<float>(speed) / 100.0f;
    ctrl->cmd_data_.chassis.y = y * static_cast<float>(speed) / 100.0f;

    ctrl->cmd_data_.ctrl_source = Component::CMD::CTRL_SOURCE_TERM;

    ctrl->cmd_tp_.Publish(ctrl->cmd_data_);

    System::Thread::Sleep(time);

    ctrl->cmd_data_.chassis.x = 0.0f;
    ctrl->cmd_data_.chassis.y = 0.0f;

    ctrl->cmd_tp_.Publish(ctrl->cmd_data_);
  } else {
    printf("参数错误\r\n");
  }

  return 0;
}
