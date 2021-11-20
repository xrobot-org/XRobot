#include "comp_init.h"

#include <stddef.h>

extern const init_t *__thread_start;
extern const init_t *__thread_end;

void init(void) {
  const size_t num_init = (__thread_end - __thread_start) / sizeof(init_t);

  /* 创建线程 */
  for (size_t j = 0; j < num_init; j++) {
    const init_t *init = __thread_start + j;
    if (init) {
      err_t err = init->dn(init->param);
      if (err) {
        (void)err;
        // TODO: Log error message;
      }
    } else {
      break;
    }
  }
}
