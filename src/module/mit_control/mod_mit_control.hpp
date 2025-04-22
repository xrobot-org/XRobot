#include "module.hpp"
#include "dev_mit_motor.hpp"

namespace Module {
class MitControl {
public:
  typedef struct Param{
    Device::MitMotor::Param mitmotor_1;
    Device::MitMotor::Param mitmotor_2;
  }Param;

  MitControl(Param& param);

  void UpdateFeedback();

  void control();
private:

  float out_;
  // float now_;
  // float dt_;
  // float last_wakeup_;

  Device::MitMotor mitmotor_1_;
  Device::MitMotor mitmotor_2_;

  System::Thread thread_;

  System::Semaphore control_lock_;
};
}  // namespace Module