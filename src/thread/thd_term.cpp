#include "dev_term.hpp"
#include "thd.hpp"

#define THD_PERIOD_MS (2)

void thd_term(void* arg) {
  runtime_t* runtime = (runtime_t*)arg;
  RM_UNUSED(runtime);

  term_init();

  while (1) {
    term_update();
  }
}
THREAD_DECLEAR(thd_term, 256, 4);
