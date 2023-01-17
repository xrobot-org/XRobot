#include "robot.hpp"

#include <comp_pid.hpp>
#include <thread.hpp>

#include "system.hpp"

/* clang-format off */
Robot::Simulator::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  },

  .imu = {
    .tp_name_prefix = "chassis",
  },

  .chassis={
    .init_g_center = 0.0f,

    .follow_pid_param = {
      .k = 0.0f,
      .p = 1.0f,
      .i = 0.5f,
      .d = 0.0f,
      .i_limit = 0.011f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -1.0f,
      .range = M_2PI,
    },

    .comp_pid_param = {
      .k = 0.0f,
      .p = 1.0f,
      .i = 0.5f,
      .d = 0.0f,
      .i_limit = 0.011f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -1.0f,
      .range = M_2PI,
    },

    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Device::TerminalController::STOP_CTRL,
        Module::RMBalance::SET_MODE_RELAX
      },
      Component::CMD::EventMapItem{
        Device::TerminalController::START_CTRL,
        Module::RMBalance::SET_MODE_FOLLOW
      }
    },

    .wheel_param = {
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.5f,
          .d = 0.0f,
          .i_limit = 0.2f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.5f,
          .d = 0.0f,
          .i_limit = 0.2f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },

    .eulr_param = Component::PID::Param{
      .k = 2.0f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.005f,
      .i_limit = 20.0f,
      .out_limit = 20.0f,
      .d_cutoff_freq = -1.0f,
      .range = M_2PI,
    },

    .gyro_param = Component::PID::Param{
      .k = 1.2f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.0f,
      .i_limit = 1.0f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -1.0f,
      .range = -1.0f,
    },

    .speed_param = Component::PID::Param{
        .k = 0.7f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 0.3f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = -1.0f,
    },

    .center_filter_cutoff_freq = 10.0f,

    .motor_param = {
      Device::RMMotor::Param{
        .model = Device::RMMotor::MOTOR_M3508,
      },
      Device::RMMotor::Param{
        .model = Device::RMMotor::MOTOR_M3508,
      },
    },
  }
};
/* clang-format on */

void robot_init() {
  auto init_thread_fn = [](void* arg) {
    static_cast<void>(arg);

    System::Init();

    Robot::Simulator blink(param);

    while (1) {
      System::Thread::Sleep(UINT32_MAX);
    }
  };

  System::Thread init_thread;

  init_thread.Create(init_thread_fn, static_cast<void*>(NULL), "init_thread_fn",
                     512, System::Thread::REALTIME);
}
