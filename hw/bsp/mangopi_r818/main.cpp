#include <cstdint>
#include <cstdio>
#include <thread.hpp>

#include "bsp.h"
#include "bsp_pwm.h"
#include "robot.hpp"

int main() {
  bsp_init();
  robot_init();
  FILE *fp = NULL;
  int temp = 0;
  while (1) {
    poll(NULL, 0, 500);
    fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    fscanf(fp, "%d", &temp);
    fclose(fp);
    if (temp < 45000) {
      bsp_pwm_set_comp(BSP_PWM_SYS_FAN, 0.024);
    } else if (temp < 60000) {
      bsp_pwm_set_comp(BSP_PWM_SYS_FAN, ((float)temp - 45000.0) / 15000.0);
    } else {
      bsp_pwm_set_comp(BSP_PWM_SYS_FAN, 1.0);
    }
  }
}
