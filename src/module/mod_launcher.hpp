/**
 * @file launcher.h
 * @author Qu Shen (503578404@qq.com)
 * @brief 弹丸发射器模块
 * @version 1.0.0
 * @date 2021-05-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_actuator.hpp"
#include "dev_referee.hpp"

namespace Module {
class Launcher {
 public:
  enum {
    LAUNCHER_ACTR_FRIC1_IDX = 0, /* 1号摩擦轮相关的索引值 */
    LAUNCHER_ACTR_FRIC2_IDX,     /* 2号摩擦轮相关的索引值 */
    LAUNCHER_ACTR_FRIC_NUM,      /* 总共的动作器数量 */
  };

  enum {
    LAUNCHER_ACTR_TRIG_IDX, /* 拨弹电机相关的索引值 */
    LAUNCHER_ACTR_TRIG_NUM, /* 总共的动作器数量 */
  };

  enum {
    LAUNCHER_CTRL_FRIC1_SPEED_IDX = 0, /* 摩擦轮1控制的速度环控制器的索引值 */
    LAUNCHER_CTRL_FRIC2_SPEED_IDX, /* 摩擦轮2控制的速度环控制器的索引值 */
    LAUNCHER_CTRL_TRIG_SPEED_IDX, /* 拨弹电机控制的速度环控制器的索引值 */
    LAUNCHER_CTRL_TRIG_ANGLE_IDX, /* 拨弹电机控制的角度环控制器的索引值 */
    LAUNCHER_CTRL_NUM,            /* 总共的控制器数量 */
  };

  typedef enum {
    LAUNCHER_MODEL_17MM = 0, /* 17mm发射机构 */
    LAUNCHER_MODEL_42MM,     /* 42mm发射机构 */
  } Model;

  typedef struct {
    float num_trig_tooth;       /* 拨弹盘中一圈能存储几颗弹丸 */
    float trig_gear_ratio;      /* 拨弹电机减速比 3508:19, 2006:36 */
    float fric_radius;          /* 摩擦轮半径，单位：米 */
    float cover_open_duty;      /* 弹舱盖打开时舵机PWM占空比 */
    float cover_close_duty;     /* 弹舱盖关闭时舵机PWM占空比 */
    Model model;                /* 发射机构型号 */
    float default_bullet_speed; /* 默认弹丸初速度 */
    uint32_t min_launch_delay;  /* 最小发射间隔(1s/最大射频) */

    Device::Actuator::PosParam trig_motor[LAUNCHER_ACTR_TRIG_NUM];
    Device::Actuator::SpeedParam fric_motor[LAUNCHER_ACTR_FRIC_NUM];
  } Param;

  /* 热量控制 */
  typedef struct {
    float heat;          /* 现在热量水平 */
    float last_heat;     /* 之前的热量水平 */
    float heat_limit;    /* 热量上限 */
    float speed_limit;   /* 弹丸初速是上限 */
    float cooling_rate;  /* 冷却速率 */
    float heat_increase; /* 每发热量增加值 */

    uint32_t available_shot; /* 热量范围内还可以发射的数量 */
  } HeatControl;

  typedef struct {
    uint32_t last_launch;    /* 上次发射器时间 单位：ms */
    bool last_fire;          /* 上次开火状态 */
    bool first_pressed_fire; /* 第一次收到开火指令 */
    uint32_t launched;       /* 已经发射的弹丸 */
    uint32_t to_launch;      /* 计划发射的弹丸 */
    uint32_t launch_delay;   /* 弹丸击发延迟 */
    float bullet_speed;      /* 弹丸初速度 */
    Component::CMD::FireMode fire_mode;
  } FireControl;

  typedef struct {
    Device::Referee::Status status;
    Device::Referee::PowerHeat power_heat;
    Device::Referee::RobotStatus robot_status;
    Device::Referee::LauncherData launcher_data;
  } RefForLauncher;

  Launcher(Param &param_);

  void UpdateFeedback();

  void Control();

  void PackOutput();

  void PackUI();

  void SetMode(Component::CMD::LauncherMode mode);

  void HeatLimit();

  void PraseRef(Device::Referee::Data &ref);

  uint32_t lask_wakeup_;

  uint32_t now_;

  float dt_;

  float trig_angle_;

  Param param_;

  Component::CMD::LauncherMode mode_; /* 发射器模式 */

  Component::CMD::LauncherCMD cmd_;

  ui_launcher_t ui_;

  /* PID计算的目标值 */
  struct {
    float fric_rpm_[2]; /* 摩擦轮电机转速，单位：RPM */
    float trig_angle_;  /* 拨弹电机角度，单位：弧度 */
  } setpoint;

  HeatControl heat_ctrl_;
  FireControl fire_ctrl_;

  Device::Actuator *trig_actuator_;
  Device::Actuator *fric_actuator_;

  RefForLauncher ref_;

  System::Thread thread_;
};
}  // namespace Module
