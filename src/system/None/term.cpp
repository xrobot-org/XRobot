#include <term.hpp>
#include <timer.hpp>

#include "bsp_usb.h"

using namespace System;

static bool connected = false;

static ms_item_t log_control;

static bool log_enable = false;

static int log_ctrl_fn(ms_item_t *item, int argc, char **argv) {
  MS_UNUSED(item);
  if (argc == 1) {
    printf("on开启/off关闭\r\n");
  } else if (argc == 2) {
    if (strcmp(argv[1], "on") == 0) {
      ms_clear();
      log_enable = true;
    } else if (strcmp(argv[1], "off") == 0) {
      log_enable = false;
    } else {
      printf("命令错误。\r\n");
    }
  } else {
    printf("命令错误。\r\n");
  }

  return 0;
}

static om_status_t print_log(om_msg_t *msg, void *arg) {
  (void)arg;

  if (!log_enable) {
    return OM_OK;
  }

  om_log_t *log = static_cast<om_log_t *>(msg->buff);

  printf("%-.4f %s", bsp_time_get(), log->data);

  return OM_OK;
}

static int term_write(const char *data, uint32_t len) {
  bsp_usb_transmit(reinterpret_cast<const uint8_t *>(data), len);
  return static_cast<int>(len);
}

Term::Term() {
  bsp_usb_init();

  ms_init(term_write);

  ms_file_init(&log_control, "log", log_ctrl_fn, NULL, NULL);
  ms_cmd_add(&log_control);

  om_config_topic(om_get_log_handle(), "d", print_log, NULL);

  auto usb_thread_fn = [](void *arg) {
    (void)arg;
    bsp_usb_update();
  };

  System::Timer::Create(usb_thread_fn, static_cast<void *>(0), 10);

  auto term_thread_fn = [](void *arg) {
    (void)arg;
    if (!bsp_usb_connect()) {
      connected = false;
      return;
    } else {
      if (!connected) {
        connected = true;
        ms_start();
      }
    }

    if (bsp_usb_avail()) {
      ms_input(bsp_usb_read_char());
    }
  };

  System::Timer::Create(term_thread_fn, static_cast<void *>(0), 10);
}
