#include <term.hpp>
#include <thread.hpp>

#include "bsp_sys.h"
#include "bsp_time.h"
#include "ms.h"
#include "om.hpp"

using namespace System;

static System::Thread term_thread;

static ms_item_t power_ctrl;

int show_fun(const char *data, size_t len) {
  while (len--) {
    putchar(*data++);
  }

  return 0;
}

static om_status_t print_log(om_msg_t *msg, void *arg) {
  XB_UNUSED(arg);

  om_log_t *log = static_cast<om_log_t *>(msg->buff);

  ms_printf_insert("%-.4f %s", static_cast<float>(bsp_time_get()) / 1000000.0f,
                   log->data);

  return OM_OK;
}

Term::Term() {
  system("stty -icanon");
  system("stty -echo");

  ms_init(show_fun);

  om_config_topic(om_get_log_handle(), "d", print_log, NULL);

  auto term_thread_fn = [](void *arg) {
    XB_UNUSED(arg);

    ms_start();

    while (1) {
      ms_input(static_cast<char>(getchar()));
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

  term_thread.Create(term_thread_fn, static_cast<void *>(0), "term_thread", 512,
                     System::Thread::LOW);
}
