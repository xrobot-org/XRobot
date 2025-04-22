#include "mod_uart_update.hpp"

void robot_init();

namespace Robot {
class Bootloader {
 public:
  typedef struct {
    Module::UartUpdate::Param uart_update;
  } Param;

  Module::UartUpdate uart_update_;

  Bootloader(Param& param) : uart_update_(param.uart_update) {}
};
}  // namespace Robot
