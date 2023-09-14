#include "bsp_pwm.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static uint64_t fan_duty_period = 50000;
static char cmd_buff[100];
static int led_dev[3];

void bsp_pwm_init() {
  if (access("/sys/class/pwm/pwmchip0/pwm0/period", W_OK)) {
    system("echo 0 > /sys/class/pwm/pwmchip0/export");
    system(
        "echo 50000 > /sys/class/pwm/pwmchip0/pwm0/period && echo 2000 > "
        "/sys/class/pwm/pwmchip0/pwm0/duty_cycle");
  }
  led_dev[0] = open("/sys/class/leds/sunxi_led0r/brightness",
                    O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
  led_dev[1] = open("/sys/class/leds/sunxi_led0g/brightness",
                    O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
  led_dev[2] = open("/sys/class/leds/sunxi_led0b/brightness",
                    O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
}

bsp_status_t bsp_pwm_start(bsp_pwm_channel_t ch) {
  if (ch == BSP_PWM_SYS_FAN) {
    system("echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable");
  }
  return BSP_OK;
}

bsp_status_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle) {
  if (ch == BSP_PWM_SYS_FAN) {
    snprintf(cmd_buff, sizeof(cmd_buff),
             "echo %lu > /sys/class/pwm/pwmchip0/pwm0/duty_cycle",
             (uint64_t)(duty_cycle * fan_duty_period));
    system(cmd_buff);
  } else {
    snprintf(cmd_buff, sizeof(cmd_buff), "%d", (uint32_t)duty_cycle * 254);
    write(led_dev[ch], cmd_buff, strlen(cmd_buff) + 1);
  }
  return BSP_OK;
}

bsp_status_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq) {
  if (ch == BSP_PWM_SYS_FAN) {
    fan_duty_period = (uint64_t)(1000000000.0 / (double)freq);
    snprintf(cmd_buff, sizeof(cmd_buff),
             "echo %lu > /sys/class/pwm/pwmchip0/pwm0/period", fan_duty_period);
    system(cmd_buff);
  }
  return BSP_OK;
}

bsp_status_t bsp_pwm_stop(bsp_pwm_channel_t ch) {
  if (ch == BSP_PWM_SYS_FAN) {
    system("echo 0 > /sys/class/pwm/pwmchip0/pwm0/enable");
  } else {
    write(led_dev[ch], "0", sizeof("0"));
  }
  return BSP_OK;
}
