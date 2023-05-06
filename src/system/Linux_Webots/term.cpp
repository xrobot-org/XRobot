#include <term.hpp>
#include <thread.hpp>

#include "bsp_time.h"
#include "ms.h"
#include "om.hpp"

using namespace System;

static System::Thread term_thread;

int show_fun(const char *data, size_t len) {
  while (len--) {
    putchar(*data++);
  }

  return 0;
}

static om_status_t print_log(om_msg_t *msg, void *arg) {
  (void)arg;

  om_log_t *log = static_cast<om_log_t *>(msg->buff);

  ms_printf_insert("%-.4f %s", bsp_time_get(), log->data);

  return OM_OK;
}

Term::Term() {
  system("stty -icanon");
  system("stty -echo");

  ms_init(show_fun);

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
