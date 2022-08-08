#include "robot.hpp"
/* clang-format off */
Robot::Infantry::Param param = {
    .chassis={
      .l1 = 0.11f,
      .l2 = 0.2f,
      .l3 = 0.31f,

      .leg_max_angle = 0.0f,

      .mech_zero = {
        2.5f,
        6.14f,
        0.0f,
        0.0f,
      },

      .leg_motor = {
        {
          .kp = 20.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 1,
          .can = BSP_CAN_1,
        },
        {
          .kp = 20.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 2,
          .can = BSP_CAN_1,
        },
        {
          .kp = 2.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 3,
          .can = BSP_CAN_1,
        },
        {
          .kp = 2.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 4,
          .can = BSP_CAN_1,
        },
      },
  },

  .bmi088_rot = {
    .rot_mat = {
      { +1, +0, +0},
      { +0, +1, +0},
      { +0, +0, +1},
    },
  },

  .bmi088_cali = {
    .gyro_offset = {
      .x = 0.000332771f,
      .y = 0.004123951f,
      .z = -0.000634991f,
    },
  },

  .cap = {
    .can = BSP_CAN_1,
    .index = DEV_CAP_FB_ID_BASE,
  },

  .cmd = {
    .sens_mouse = 0.06f,
    .sens_stick = 6.0f,
    .key_map = {
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_W},        /* 向前 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_S},        /* 向后 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_A},        /* 向左 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_D},        /* 向右 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_SHIFT},    /* 加速 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_CTRL},     /* 减速 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_L_CLICK},  /* 开火 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_R_CLICK}, /* 切换开火模式 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_E},       /* 自瞄模式 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_F},       /* 弹舱盖开关 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_R},       /* 小陀螺模式 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_G},       /* 反转拨弹 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_C}, },    /* 跟随云台呈35度 */

    .move = {
      .sense_norm = 0.8f,
      .sense_fast = 1.25f,
      .sense_slow = 0.8f,
    },

    .default_mode = {
      .gimbal = Component::CMD::GIMBAL_MODE_ABSOLUTE,
      .chassis = Component::CMD::CHASSIS_MODE_FOLLOW_GIMBAL,
      .launcher = Component::CMD::LAUNCHER_MODE_LOADED,
    }
  },



};
/* clang-format on */

void robot_init() {
  auto init_thread_fn = [](void* arg) {
    RM_UNUSED(arg);

    Robot::Infantry infantry(param, 500.0f);
    while (1) {
      System::Thread::Sleep(UINT32_MAX);
    }
  };

  System::Thread init_thread;
  THREAD_DECLEAR(init_thread, init_thread_fn, 1024, System::Thread::Realtime,
                 NULL);
}
