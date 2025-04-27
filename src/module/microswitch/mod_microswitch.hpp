#include "dev_can.hpp"
#include "module.hpp"

namespace Module {
class MicroSwitch {
 public:
  typedef enum { SWITCH_1, SWITCH_2, SWITCH_3, SWITCH_4, SWITCH_NUM } SwitchID;

  typedef enum { OFF, ON } Status;

  MicroSwitch();

  void UpdatePinStatus();

  void TransData();

  static int SetCMD(MicroSwitch* imu, int argc, char** argv);

 private:
  std::array<uint8_t, SWITCH_NUM> gpio_status_;

  System::Database::Key<uint32_t> can_id_;

  System::Database::Key<uint32_t> on_send_delay_;

  System::Database::Key<uint32_t> off_send_delay_;

  System::Term::Command<MicroSwitch*> cmd_;

  uint32_t last_send_time_ = 0;

  Status status_ = OFF, last_status_ = OFF;

  Device::Can::Pack send_buff_;
};
}  // namespace Module
