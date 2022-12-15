#include "term.hpp"

#include "thread.hpp"

using namespace System;

static System::Thread term_thread_;

int show_fun(const char *data, uint32_t len) {
  while (len--) {
    putchar(*data++);
  }

  return 0;
}

Term::Term() {
  system("stty -icanon");
  system("stty -echo");

  auto term_thread = [](void *arg) {
    (void)arg;

    ms_init(show_fun);

    ms_start();

    while (1) {
      ms_input(getchar());
    }
  };

  term_thread_.Create(term_thread, (void *)0, "term_thread", 512,
                      System::Thread::Low);
}
