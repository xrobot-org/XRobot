
#include "comp_pid.hpp"
#include "dev_rm_motor.hpp"

namespace Module {
class SpeedControl {
 public:
  typedef struct {
    Component::PID::Param pid;
    Device::RMMotor::Param motor;
  } Param;

  /* 构造函数 */
  SpeedControl(Param& param);

  /* 控制函数 */
  void Control();

  /* 更新电机反馈 */
  void UpdateFeedback();

 private:
  float now_;         /* 当前时间 */
  float dt_;          /* 距离上次控制的时间 */
  float lask_wakeup_; /* 上次控制的时间 */

  float speed_setpoint_; /* 速度目标值 */
  float output_;         /* 电机输出 */

  Component::PID pid_;    /* pid对象 */
  Device::RMMotor motor_; /* motor对象 */

  System::Thread thread_; /* 控制线程 */
};
}  // namespace Module
