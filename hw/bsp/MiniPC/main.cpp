#include <thread.hpp>

#include "bsp.h"
#include "robot.hpp"

int main() {
  bsp_init();
  robot_init();
  while (1) {
    poll(NULL, 0, UINT32_MAX);
  }
}
