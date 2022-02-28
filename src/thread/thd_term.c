#include "dev_term.h"
#include "thd.h"

#define THD_PERIOD_MS (2)

void thd_term(void* arg) {
  runtime_t* runtime = arg;
  RM_UNUSED(runtime);

  term_init();

  while (1) {
    term_update();
  }
}
THREAD_DECLEAR(thd_term, 256, 4);
