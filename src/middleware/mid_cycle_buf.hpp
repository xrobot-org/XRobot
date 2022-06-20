#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 循环缓冲区 */
typedef struct {
  uint32_t in;
  uint32_t out;
  uint32_t size;
  size_t ele_size;
  void *data;
} cycle_buf_t;

bool cycle_buf_alloc(cycle_buf_t *cbuf, uint32_t size, size_t ele_size);
bool cycle_buf_free(cycle_buf_t *cbuf);
size_t cycle_buf_in(cycle_buf_t *cbuf, const void *buf, size_t len);
size_t cycle_buf_out(cycle_buf_t *cbuf, void *buf, size_t len);
