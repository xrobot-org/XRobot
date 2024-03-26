#pragma once

#include <module.hpp>

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"

namespace Module {
class Launcher {
 public:
  /* 发射器运行模式 */
  typedef enum {
    RELAX,  /* 放松模式，电机不输出 */
    SAFE,   /* 保险模式，电机闭环控制保持静止 */
    LOADED, /* 上膛模式，摩擦轮开启。随时准备开火 */
  } FireMode;

  /* 开火模式 */
  typedef enum {
    SINGLE,    /* 单发开火模式  */
    BURST,     /* N爆发开火模式 */
    CONTINUED, /* 持续开火模式 */
  } TrigMode;

  typedef enum { OPEN, CLOSE } CoverMode;

  typedef enum {
    CHANGE_FIRE_MODE_RELAX,
    CHANGE_FIRE_MODE_SAFE,
    CHANGE_FIRE_MODE_LOADED,
    CHANGE_TRIG_MODE_SINGLE,
    CHANGE_TRIG_MODE_BURST,
    CHANGE_TRIG_MODE,
    OPEN_COVER,
    CLOSE_COVER,
    LAUNCHER_START_FIRE, /* 开火，拨弹盘开始发弹 */
  } LauncherEvent;

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

    std::array<Component::PosActuator::Param, LAUNCHER_ACTR_TRIG_NUM> trig_actr;
    std::array<Component::SpeedActuator::Param, LAUNCHER_ACTR_FRIC_NUM>
        fric_actr;
    std::array<Device::RMMotor::Param, LAUNCHER_ACTR_TRIG_NUM> trig_motor;
    std::array<Device::RMMotor::Param, LAUNCHER_ACTR_FRIC_NUM> fric_motor;

    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;
  } Param;

  /* 热量控制 */
  struct HeatControl {
    float heat;          /* 现在热量水平 */
    float last_heat;     /* 之前的热量水平 */
    float heat_limit;    /* 热量上限 */
    float speed_limit;   /* 弹丸初速是上限 */
    float cooling_rate;  /* 冷却速率 */
    float heat_increase; /* 每发热量增加值 */

    uint32_t available_shot; /* 热量范围内还可以发射的数量 */
  };

  struct FireControl {
    bool fire = false;
    bool stall = false;
    uint32_t last_launch = 0; /* 上次发射器时间 单位：ms */
    bool last_fire = false;   /* 上次开火状态 */
    float last_trig_angle = 1.0f;
    bool first_pressed_fire;      /* 第一次收到开火指令 */
    uint32_t launched;            /* 已经发射的弹丸 */
    uint32_t to_launch;           /* 计划发射的弹丸 */
    uint32_t launch_delay;        /* 弹丸击发延迟 */
    float bullet_speed;           /* 弹丸初速度 */
    TrigMode trig_mode_ = SINGLE; /* 发射器模式 */
    FireMode fire_mode_ = RELAX;
  };

  typedef struct {
    Device::Referee::Status status;
    Device::Referee::PowerHeat power_heat;
    Device::Referee::RobotStatus robot_status;
    Device::Referee::LauncherData launcher_data;
  } RefForLauncher;

  Launcher(Param &param, float control_freq);

  void UpdateFeedback();

  void Control();

  void PackOutput();

  void SetTrigMode(TrigMode mode);

  void SetFireMode(FireMode mode);

  void HeatLimit();

  float LimitLauncherFreq();

  void PraseRef();

  static void DrawUIStatic(Launcher *launcher);

  static void DrawUIDynamic(Launcher *launcher);

 private:
  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;

  float dt_ = 0.0f;

  float trig_angle_ = 0.0f;

  Param param_;

  CoverMode cover_mode_ = CLOSE; /* 弹舱盖模式 */

  /* PID计算的目标值 */
  struct {
    std::array<float, 2> fric_rpm_; /* 摩擦轮电机转速，单位：RPM */
    float trig_angle_ = 0.0f;       /* 拨弹电机角度，单位：弧度 */
  } setpoint_;

  HeatControl heat_ctrl_;
  FireControl fire_ctrl_;

  std::array<Component::PosActuator *, LAUNCHER_ACTR_TRIG_NUM> trig_actuator_;
  std::array<Component::SpeedActuator *, LAUNCHER_ACTR_FRIC_NUM> fric_actuator_;

  std::array<Device::RMMotor *, LAUNCHER_ACTR_TRIG_NUM> trig_motor_;
  std::array<Device::RMMotor *, LAUNCHER_ACTR_FRIC_NUM> fric_motor_;

  RefForLauncher ref_;

  System::Thread thread_;

  System::Semaphore ctrl_lock_;

  Device::Referee::Data raw_ref_;

  Component::UI::String string_;

  Component::UI::Rectangle rectangle_;

  Component::UI::Arc arc_;
};
}  // namespace Module
