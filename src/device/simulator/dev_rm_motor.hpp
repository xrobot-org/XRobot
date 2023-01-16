#include "dev_motor.hpp"

namespace Device {
class RMMotor : public BaseMotor {
 public:
  typedef enum {
    MOTOR_NONE = 0,
    MOTOR_M2006,
    MOTOR_M3508,
    MOTOR_GM6020,
  } Model;

  typedef struct {
    Model model;
  } Param;

  RMMotor(const Param& param, const char* name);

  void Control(float output);

  bool Update();

 private:
  Param param_;
};
}  // namespace Device
