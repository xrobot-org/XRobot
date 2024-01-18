#include <cstdint>
#include <database.hpp>
#include <functional>
#include <memory.hpp>
#include <queue.hpp>
#include <semaphore.hpp>
#include <term.hpp>
#include <thread.hpp>
#include <timer.hpp>

#include "bsp_flash.h"
#include "bsp_sys.h"
#include "bsp_time.h"
#include "om.hpp"

namespace System {
template <typename RobotType, typename... RobotParam>
void Start(RobotParam... param) {
  new Term();

  new Message();

  printf("\r\n--------------------------------\r\n");
  printf("XRobot bootloader\r\n");

  static auto xrobot_debug_handle = new RobotType(param...);

  XB_UNUSED(xrobot_debug_handle);

  printf("\r\n--------------------------------\r\n");
  printf("Start jump to app.\r\n");
  printf("App info: addr 0x%x size %d\r\n", BSP_FLASH_APP_ADDR,
         BSP_FLASH_APP_SIZE);
  bsp_sys_jump_app();
}
}  // namespace System
