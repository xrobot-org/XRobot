#include "bsp.h"
#include "robot.hpp"
#include "thread.hpp"

int main() {
  bsp_init();
  robot_init();
  System::Thread::StartKernel();
}
