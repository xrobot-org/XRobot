#include <thread.hpp>
#include <vector>

#include "comp_cmd.hpp"
#include "dev_mech.hpp"
#include "dev_rm_motor.hpp"
#include "module.hpp"

namespace Module {
class OreCollect {
 public:
  typedef struct {
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param x_actr;

    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param pitch_actr;

    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param pitch_1_actr;

    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param yaw_actr;

    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                2>::Param z_actr;

    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                2>::Param z_1_actr;

    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                2>::Param y_actr;
  } Param;

  typedef struct {
    float x;
    float pitch;
    float pitch_1;
    float yaw;
    float z;
    float z_1;
    float y;
  } Setpoint;

  typedef enum {
    RESET,
    STEP_1,  /* 对准地面矿 */
    STEP_2,  /* 贴近地面矿 */
    STEP_3,  /* 放入存矿区 */
    STEP_4,  /* 对准小资源岛矿 */
    STEP_5,  /* 贴近小资源岛 */
    STEP_6,  /* 放入存矿区（边缘） */
    STEP_7,  /* 下降高度 */
    STEP_8,  /* 对准兑换站 */
    STEP_9,  /* 进入兑换站 */
    STEP_10, /* 拿矿 */
    START,
    STOP,
  } Event;

  OreCollect(Param& param, float control_freq);

  void Control();

  void UpdateFeedback();

 private:
  Param& param_;

  float dt_;

  float last_wakeup_;

  float now_;

  Setpoint setpoint_;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1>
      x_actr_;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1>
      pitch_actr_;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1>
      pitch_1_actr_;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1>
      yaw_actr_;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 2>
      z_actr_;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 2>
      z_1_actr_;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 2>
      y_actr_;

  System::Thread thread_;
};
}  // namespace Module
