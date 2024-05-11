#include "bsp_uart.h"
#include "dev_mt6701.hpp"
#include "module.hpp"

static char uart_tx_buff[1024] = {0};

namespace Module {
class CustomController {
 public:
  typedef struct {
    Device::MT6701::Param mt6701[6];
  } Param;
  CustomController(Param& param)
      : mt6701_1(param.mt6701[0]),
        mt6701_2(param.mt6701[1]),
        mt6701_3(param.mt6701[2]),
        mt6701_4(param.mt6701[3]),
        mt6701_5(param.mt6701[4]),
        mt6701_6(param.mt6701[5]) {
    auto task_fun = [](CustomController* ctrl) {
      (void)snprintf(
          uart_tx_buff, sizeof(uart_tx_buff), "%f,%f,%f,%f,%f,%f\n",
          ctrl->mt6701_1.angle_.Value(), ctrl->mt6701_2.angle_.Value(),
          ctrl->mt6701_3.angle_.Value(), ctrl->mt6701_4.angle_.Value(),
          ctrl->mt6701_5.angle_.Value(), ctrl->mt6701_6.angle_.Value());

      bsp_uart_transmit(BSP_UART_MCU, reinterpret_cast<uint8_t*>(uart_tx_buff),
                        strlen(uart_tx_buff), false);
    };

    System::Timer::Create(task_fun, this, 10);
  }

  Device::MT6701 mt6701_1;
  Device::MT6701 mt6701_2;
  Device::MT6701 mt6701_3;
  Device::MT6701 mt6701_4;
  Device::MT6701 mt6701_5;
  Device::MT6701 mt6701_6;
};
}  // namespace Module
