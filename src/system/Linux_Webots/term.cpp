#include <term.hpp>
#include <thread.hpp>

using namespace System;

static System::Thread term_thread;

int show_fun(const char *data, uint32_t len) {
  while (len--) {
    putchar(*data++);
  }

  return 0;
}

Term::Term() {
  system("stty -icanon");
  system("stty -echo");

  ms_init(show_fun);

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
