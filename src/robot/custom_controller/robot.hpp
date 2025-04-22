/* #include "dev_xxx.hpp" */
#include "bsp_uart.h"
#include "mod_custom_controller.hpp"

void robot_init();

namespace Robot {
class CustomController {
 public:
  typedef struct {
  } Param;
  Module::CustomController cust_ctrl{};
  CustomController(Param& param) { XB_UNUSED(param); }
};
}  // namespace Robot
