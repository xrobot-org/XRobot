#include <vector>

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "dev_mech.hpp"
#include "dev_rm_motor.hpp"
#include "module.hpp"

namespace Module {
class DartLauncher {
 public:
  typedef struct {
    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param rod_actr;
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    std::array<Component::SpeedActuator::Param, 4> fric_actr;
    std::array<Device::RMMotor::Param, 4> fric_motor;
  } Param;

  typedef struct {
    float rod_position;
    float fric_speed;
  } Setpoint;
  typedef enum {
    FRIC_OUTOST, /*摩擦轮前哨战模式*/
    FRIC_BASE,   /*摩擦轮基地模式*/
    FRIC_OFF,
  } FricMode;
  typedef enum {
    ADVANCE, /* 推进1个单位 */
    STAY,
    BACK, /* 回到初始位置0,1 */
  } RodMode;
  typedef enum {
    RELAX, /* 摩擦轮不输出，回到初始位置0.1 */
    SET_FRIC_OFF,
    SET_STAY,
    RESET_POSITION,  /* 回到初始位置0.1 */
    FIRE,            /* 发射 */
    SET_FRIC_OUTOST, /*攻击前哨站*/
    SET_FRIC_BASE,   /*攻击基地*/
  } DartEvent;       /* 事件 */

  DartLauncher(Param& param, float control_freq);

  void UpdateFeedback();

  void Control();
  void SetRodMode(RodMode mode);
  void SetFricMode(FricMode mode);

 private:
  System::Semaphore ctrl_lock_;

  Param& param_;

  FricMode fric_mode_;
  RodMode rod_mode_;

  Setpoint setpoint_;

  float dt_ = 0.0f;

  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;
  int rod_position_ = 0; /* 丝杆位置0,1,2,3循环 */

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1>
      rod_actr_; /* 内含电机、执行器、防堵数据 */
  std::array<Component::SpeedActuator*, 4> fric_actr_;
  std::array<Device::RMMotor*, 4> fric_motor_;

  std::array<float, 4> motor_out_;

  System::Thread thread_;
};
}  // namespace Module
