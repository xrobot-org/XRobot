#include "robot.hpp"

#include "system.hpp"
#include "thread.hpp"

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
      .type = Component::Mixer::MECANUM,

      .follow_pid_param = {
      .k = 0.5f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.0f,
      .i_limit = 1.0f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -1.0f,
      .range = M_2PI,
    },

    .event_map = {
      Component::CMD::CreateMapItem(
        Device::TerminalController::Stop,
        Module::RMChassis::ChangeModeRelax
      ),
      Component::CMD::CreateMapItem(
        Device::TerminalController::Start,
        Module::RMChassis::ChangeModeIndenpendent
      ),
    },

    .actuator_param = {
      {
        .speed = {
          .k = 0.00005f,
          .p = 1.0f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.02f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,

      },
      {
        .speed = {
          .k = 0.00005f,
          .p = 1.0f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.02f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      {
        .speed = {
          .k = 0.00005f,
          .p = 1.0f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.02f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      {
        .speed = {
          .k = 0.00005f,
          .p = 1.0f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.02f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },

    .motor_param = {
      {
          .model = Device::RMMotor::MOTOR_M3508,
      },
      {
          .model = Device::RMMotor::MOTOR_M3508,
      },
      {
          .model = Device::RMMotor::MOTOR_M3508,
      },
      {
          .model = Device::RMMotor::MOTOR_M3508,
      },
    },
  },
};
/* clang-format on */

void robot_init() {
  auto init_thread_fn = [](void* arg) {
    RM_UNUSED(arg);

    System::Init();

    Robot::Simulator blink(param);

    while (1) {
      System::Thread::Sleep(UINT32_MAX);
    }
  };

  System::Thread init_thread;

  init_thread.Create(init_thread_fn, (void*)0, "init_thread_fn", 512,
                     System::Thread::Realtime);
}
