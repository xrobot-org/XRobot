#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"
#include "module.hpp"

#define TRIG_NUM 1
#define FRIC_NUM 2

namespace Module {
class DroneLauncher {
 public:
  typedef enum {
    SET_RELAX,
    CHANGE_FRIC_MODE_SAFE,
    CHANGE_FRIC_MODE_LOADED,
    CHANGE_TRIG_MODE_SINGLE,
    CHANGE_TRIG_MODE_BURST,
    CHANGE_TRIG_MODE_CONTINUED,
    SET_START_FIRE,
  } Event;
  typedef enum {
    SAFE,
    LOADED,
  } FricMode;
  typedef enum {
    RELAX,
    SINGLE,
    BURST,
    CONTINUED,
  } TrigMode;
  typedef struct {
    float trig_gear_ratio;
    float bullet_circle_num;
    uint32_t min_launcher_delay;
    std::array<Component::PosActuator::Param, TRIG_NUM> trig_actr;
    std::array<Device::RMMotor::Param, TRIG_NUM> trig_motor;
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;
  } Param;
  typedef struct {
    float trig_pos;
  } Setpoint;
  typedef struct {
    Device::Referee::Status status;
    Device::Referee::RobotStatus robot_status;
  } RefForLauncher;
  DroneLauncher(Param& param, float control_freq);
  void SetTrigMode(TrigMode mode);
  void SetFricMode(FricMode mode);
  void Feedback();
  void Control();
  void FricControl();
  void TrigControl();
  void PraseRef();

 private:
  float dt_;
  uint64_t now_;
  uint64_t last_wakeup_;

  Param& param_;
  Setpoint setpoint_;

  TrigMode trigmode_ = RELAX;
  FricMode fricmode_ = SAFE;

  RefForLauncher ref_;
  Device::Referee::Data raw_ref_;

  System::Thread thread_;
  System::Semaphore ctrl_lock_;

  float trig_out_ = 0.0f;

  float trig_pos_ = 0.0f;
  float trig_last_pos_ = 1.0f;

  float continued_rotation_speed_;

  uint32_t last_launch_time_;
  uint32_t launch_delay_;

  float trig_set_freq_;

  bool stall_ = false;

  std::array<Component::PosActuator*, TRIG_NUM> trig_actr_;
  std::array<Device::RMMotor*, TRIG_NUM> trig_motor_;
};
}  // namespace Module
