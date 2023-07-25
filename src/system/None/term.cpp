#include <term.hpp>
#include <timer.hpp>

#include "bsp_sys.h"
#include "bsp_usb.h"

using namespace System;

static bool connected = false;

static ms_item_t power_ctrl;

static om_status_t print_log(om_msg_t *msg, void *arg) {
  XB_UNUSED(arg);

  if (!bsp_usb_connect()) {
    return OM_ERROR_NOT_INIT;
  }

  om_log_t *log = static_cast<om_log_t *>(msg->buff);

  ms_printf_insert("%-.4f %s", bsp_time_get(), log->data);

  return OM_OK;
}

static int term_write(const char *data, size_t len) {
  bsp_usb_transmit(reinterpret_cast<const uint8_t *>(data), len);
  return static_cast<int>(len);
}

Term::Term() {
  bsp_usb_init();

  ms_init(term_write);

  om_config_topic(om_get_log_handle(), "d", print_log, NULL);

  auto usb_thread_fn = [](void *arg) {
    XB_UNUSED(arg);
    bsp_usb_update();
  };

  System::Timer::Create(usb_thread_fn, static_cast<void *>(0), 10);

  auto term_thread_fn = [](void *arg) {
    XB_UNUSED(arg);
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

  auto pwr_cmd_fn = [](ms_item_t *item, int argc, char **argv) {
    XB_UNUSED(item);

    if (argc == 1) {
      printf("Please add option:shutdown reboot sleep or stop.\r\n");
    } else if (argc == 2) {
      if (strcmp(argv[1], "sleep") == 0) {
        bsp_sys_sleep();
      } else if (strcmp(argv[1], "stop") == 0) {
        bsp_sys_stop();
      } else if (strcmp(argv[1], "shutdown") == 0) {
        bsp_sys_shutdown();
      } else if (strcmp(argv[1], "reboot") == 0) {
        bsp_sys_reset();
      }
    }

    return 0;
  };

  ms_file_init(&power_ctrl, "power", pwr_cmd_fn, NULL, 0, false);
  ms_cmd_add(&power_ctrl);

  System::Timer::Create(term_thread_fn, static_cast<void *>(0), 10);
}
