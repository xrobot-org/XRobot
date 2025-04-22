/* #include "dev_xxx.hpp" */
#include "mod_mit_control.hpp"
void robot_init();

namespace Robot {
class MitControl {
public:
  typedef struct {
    Module::MitControl::Param mitcontrol;
  }Param;

  MitControl(Param& param):mit_control_(param.mitcontrol){}

  Device::Can can_;

  Module::MitControl mit_control_;
};
}  // namespace Robot