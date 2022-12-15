#include "bsp_time.h"

#include <sys/time.h>

uint32_t bsp_time_get_ms() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint32_t bsp_time_get_us() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

float bsp_time_get() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + ((float)tv.tv_usec / 1000000.0f);
}
