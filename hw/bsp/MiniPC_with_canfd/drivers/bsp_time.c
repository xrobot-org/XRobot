#include "bsp_time.h"

#include <sys/time.h>

static struct timeval start_time;

void bsp_time_init() { gettimeofday(&start_time, NULL); }

uint32_t bsp_time_get_ms() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((tv.tv_sec - start_time.tv_sec) * 1000 +
          (tv.tv_usec - start_time.tv_usec) / 1000) %
         UINT32_MAX;
}

uint64_t bsp_time_get_us() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((tv.tv_sec - start_time.tv_sec) * 1000000 +
          (tv.tv_usec - start_time.tv_usec)) %
         UINT32_MAX;
}

uint64_t bsp_time_get() __attribute__((alias("bsp_time_get_us")));
