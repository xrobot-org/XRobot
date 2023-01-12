#include "comp_utils.hpp"
#include "database.hpp"
#include "dev_blink_led.hpp"
#include "term.hpp"
#include "timer.hpp"

void robot_init();
namespace Robot {
class Blink {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
  } Param;

  Message message_;

  System::Term term_;
  System::Database database_;
  System::Timer timer_;

  Device::BlinkLED led_;

  Blink(Param& param) : led_(param.led) {}
};
}  // namespace Robot
