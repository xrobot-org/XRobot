#include "mid_cycle_buf.hpp"

#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "comp_utils.hpp"

static uint32_t cycle_buf_unused(cycle_buf_t *cbuf) {
  return (cbuf->size) - (cbuf->in - cbuf->out);
}

static void cycle_buf_copy_in(cycle_buf_t *cbuf, const void *src, uint32_t len,
                              uint32_t off) {
  ASSERT(cbuf);
  ASSERT(src);

  uint32_t size = cbuf->size;
  size_t ele_size = cbuf->ele_size;

  off %= cbuf->size;
  if (ele_size != 1) {
    off *= ele_size;
    size *= ele_size;
    len *= ele_size;
  }
  uint32_t l = MIN(len, size - off);

  memcpy(cbuf->data + off, src, l);
  memcpy(cbuf->data, src + l, len - l);
}

static void cycle_buf_copy_out(cycle_buf_t *cbuf, void *dst, uint32_t len,
                               uint32_t off) {
  ASSERT(cbuf);
  ASSERT(dst);

  uint32_t size = cbuf->size;
  size_t ele_size = cbuf->ele_size;

  off %= cbuf->size;
  if (ele_size != 1) {
    off *= ele_size;
    size *= ele_size;
    len *= ele_size;
  }
  uint32_t l = MIN(len, size - off);

  memcpy(dst, cbuf->data + off, l);
  memcpy(dst + l, cbuf->data, len - l);
}

bool cycle_buf_alloc(cycle_buf_t *cbuf, uint32_t size, size_t ele_size) {
  ASSERT(cbuf);
  ASSERT(size);

  cbuf->in = 0;
  cbuf->out = 0;
  cbuf->ele_size = ele_size;

  cbuf->data = pvPortMalloc(size * ele_size);

  if (!cbuf->data) {
    cbuf->size = 0;
    return false;
  }
  cbuf->size = size;
  return true;
}

bool cycle_buf_free(cycle_buf_t *cbuf) {
  if (cbuf == NULL) return false;
  vPortFree(cbuf->data);
  cbuf->in = 0;
  cbuf->out = 0;
  cbuf->ele_size = 0;
  cbuf->data = NULL;
  cbuf->size = 0;
  return true;
}

size_t cycle_buf_in(cycle_buf_t *cbuf, const void *buf, size_t len) {
  if (len > cbuf->size) len = cbuf->size;
  cycle_buf_copy_in(cbuf, buf, len, cbuf->in);
  cbuf->in += len;
  uint32_t l = cycle_buf_unused(cbuf);
  if (len > l) cbuf->out += len - l;
  return len;
}

size_t cycle_buf_out(cycle_buf_t *cbuf, void *buf, size_t len) {
  uint32_t l = cbuf->in - cbuf->out;
  if (len > l) len = l;
  cycle_buf_copy_out(cbuf, buf, len, cbuf->out);
  cbuf->out += len;
  return len;
}
