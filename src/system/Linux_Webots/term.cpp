#include <term.hpp>
#include <thread.hpp>

#include "bsp_time.h"
#include "ms.h"
#include "om.hpp"

using namespace System;

static System::Thread term_thread;

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

int show_fun(const char *data, uint32_t len) {
  if (log_enable) {
    return OM_OK;
  }

  while (len--) {
    putchar(*data++);
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

Term::Term() {
  system("stty -icanon");
  system("stty -echo");

  ms_init(show_fun);

  ms_file_init(&log_control, "log", log_ctrl_fn, NULL, NULL);
  ms_cmd_add(&log_control);

  om_config_topic(om_get_log_handle(), "d", print_log, NULL);

  auto term_thread_fn = [](void *arg) {
    (void)arg;

    ms_start();

    while (1) {
      ms_input(static_cast<char>(getchar()));
    }
  };

  term_thread.Create(term_thread_fn, static_cast<void *>(0), "term_thread", 512,
                     System::Thread::LOW);
}
