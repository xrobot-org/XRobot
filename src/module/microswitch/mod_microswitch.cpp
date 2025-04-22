#include "mod_microswitch.hpp"

#include "bsp_can.h"
#include "bsp_gpio.h"
#include "bsp_time.h"
#include "dev_can.hpp"
using namespace Module;

MicroSwitch::MicroSwitch()
    : can_id_("sw_can_id", 0x02),
      on_send_delay_("sw_on_delay", 2),
      off_send_delay_("sw_off_delay", 100),
      cmd_(this, SetCMD, "set_switch") {
  auto microswitch_thread = [](MicroSwitch *microswitch) {
    microswitch->UpdatePinStatus();
    microswitch->TransData();
  };

  System::Timer::Create(microswitch_thread, this, 1);
}

void MicroSwitch::UpdatePinStatus() {
  gpio_status_[SWITCH_1] = bsp_gpio_read_pin(BSP_GPIO_SWITCH1);
  gpio_status_[SWITCH_2] = bsp_gpio_read_pin(BSP_GPIO_SWITCH2);
  gpio_status_[SWITCH_3] = bsp_gpio_read_pin(BSP_GPIO_SWITCH3);
  gpio_status_[SWITCH_4] = bsp_gpio_read_pin(BSP_GPIO_SWITCH4);

  status_ = OFF;

  for (auto gpio : gpio_status_) {
    if (gpio) {
      status_ = ON;
    }
  }

  if (status_ != last_status_) {
    this->last_send_time_ = 0;
    last_status_ = status_;
  }
}

void MicroSwitch::TransData() {
  uint32_t delay = 0;
  if (this->status_ == OFF) {
    delay = off_send_delay_.data_;
  } else {
    delay = on_send_delay_.data_;
  }

  if (bsp_time_get_ms() - last_send_time_ >= delay) {
    last_send_time_ = bsp_time_get_ms();
    for (int i = 0; i < SWITCH_NUM; i++) {
      send_buff_.data[i * 2] = i;
      send_buff_.data[i * 2 + 1] = gpio_status_[i];
    }
    send_buff_.index = can_id_.data_;

    Device::Can::SendStdPack(BSP_CAN_1, send_buff_);
  }
}

int MicroSwitch::SetCMD(MicroSwitch *microswitch, int argc, char **argv) {
  if (argc == 1) {
    printf("set_can_id     [id]    设置can id\r\n");
    printf("set_off_delay  [time]  设置开关未闭合时发送延时ms\r\n");
    printf("set_on_delay   [time]  设置开关闭合时发送延时ms\r\n");
  } else if (argc == 3 && strcmp(argv[1], "set_off_delay") == 0) {
    int delay = std::stoi(argv[2]);

    if (delay > 1000) {
      delay = 1000;
    }

    if (delay < 1) {
      delay = 1;
    }

    microswitch->off_send_delay_.Set(delay);

    printf("off delay:%d\r\n", delay);
  } else if (argc == 3 && strcmp(argv[1], "set_on_delay") == 0) {
    int delay = std::stoi(argv[2]);

    if (delay > 1000) {
      delay = 1000;
    }

    if (delay < 1) {
      delay = 1;
    }

    microswitch->on_send_delay_.Set(delay);

    printf("on delay:%d\r\n", delay);
  } else if (argc == 3 && strcmp(argv[1], "set_can_id") == 0) {
    int id = std::stoi(argv[2]);

    microswitch->can_id_.Set(id);

    printf("can id:%d\r\n", id);

  } else {
    printf("命令错误\r\n");
  }

  return 0;
}
