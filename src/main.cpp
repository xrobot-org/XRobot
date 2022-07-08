#include "bsp.h"
#include "robot.hpp"
#include "thread.hpp"

int main(void) {
  /* Init Clock and Peripheral */
  bsp_init();

  /* Init task */
  robot_init();

  /* Start scheduler */
  System::Thread::StartKernel();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  while (1) {
  }
}
