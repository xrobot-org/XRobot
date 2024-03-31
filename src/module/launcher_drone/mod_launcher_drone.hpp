#include "module.hpp"
#pragma once

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"

namespace Module {
class UVALauncher {
 public:
  typedef enum {
    SINGLE, /* 单发开火模式  */
    BURST,  /* N爆发开火模式 */
    SAFE,
    FIRE
  } TrigMode;

  typedef struct {
    float trig_gear_ratio; /* 拨弹电机减速比 3508:19, 2006:36 */

    float num_trig_tooth; /* 拨弹盘中一圈能存储几颗弹丸 */

    uint32_t min_launch_delay; /* 最小发射间隔(1s/最大射频) */

    std::array<Component::PosActuator::Param, 1> trig_actr;

    std::array<Device::RMMotor::Param, 1> trig_motor;

    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;
  } Param;

  typedef enum {
    CHANGE_FIRE_MODE_RELAX,
    CHANGE_FIRE_MODE_SAFE,
    CHANGE_FIRE_MODE_LOADED,
    CHANGE_TRIG_MODE_SINGLE,
    CHANGE_TRIG_MODE_BURST,
    CHANGE_TRIG_MODE,
    OPEN_COVER,
    CLOSE_COVER,
    LAUNCHER_START_FIRE,
  } LauncherEvent;

  enum {
    LAUNCHER_ACTR_TRIG_IDX, /* 拨弹电机相关的索引值 */
    LAUNCHER_ACTR_TRIG_NUM, /* 总共的动作器数量 */
  };

  struct FireControl {
    bool fire = false;
    bool firc_on = false;
    uint32_t last_launch = 0; /* 上次发射器时间 单位：ms */
    bool last_fire = false;   /* 上次开火状态 */
    float last_trig_angle = 1.0f;
    bool first_pressed_fire;    /* 第一次收到开火指令 */
    uint32_t to_launch = 0;     /* 计划发射的弹丸 */
    uint32_t launch_delay;      /* 弹丸击发延迟 */
    TrigMode trig_mode_ = SAFE; /* 发射器模式 */
  };
  UVALauncher(Param &param, float control_freq);

  typedef struct {
    Device::Referee::Status status;

    Device::Referee::RobotStatus robot_status;

  } RefForLauncher;

  void Control();

  void SetTrigMode(TrigMode mode);

  void PraseRef();

  void FricControl();

 private:
  float last_wakeup_ = 0;

  float now_ = 0;

  float dt_ = 0;

  float trig_angle_ = 0;

  Param param_;

  struct {
    float trig_angle_ = 0; /* 拨弹电机角度，单位：弧度 */
  } setpoint_;

  FireControl fire_ctrl_;

  RefForLauncher ref_;

  Device::Referee::Data raw_ref_;

  std::array<Component::PosActuator *, 1> trig_actuator_;

  std::array<Device::RMMotor *, 1> trig_motor_;

  System::Thread thread_;

  System::Semaphore ctrl_lock_;
};
}  // namespace Module
