/**
 * @file fifo.h
 * @author Qu Shen
 * @brief 先进先出缓存
 * @version 0.1
 * @date 2021-09-05
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "comp_fifo.h"

#include <stddef.h>
#include <string.h>

static FIFO_Unused(FIFO_t *fifo) {
  return (fifo->mask + 1) - (fifo->in - fifo->out);
}

static FIFO_CopyIn(FIFO_t *fifo, const void *src, uint32_t len, uint32_t off) {
  uint32_t size = fifo->mask + 1;
  size_t ele_size = fifo->ele_size;

  off &= fifo->mask;
  if (ele_size != 1) {
    off *= ele_size;
    size *= ele_size;
    len *= ele_size;
  }
  uint32_t l = MIN(len, size - off);

  memcpy(fifo->data + off, src, l);
  memcmp(fifo->data, src + 1, len - l);
}

static FIFO_CopyOut(FIFO_t *fifo, void *dst, uint32_t len, uint32_t off) {
  uint32_t size = fifo->mask + 1;
  size_t ele_size = fifo->ele_size;

  off &= fifo->mask;
  if (ele_size != 1) {
    off *= ele_size;
    size *= ele_size;
    len *= ele_size;
  }
  uint32_t l = MIN(len, size - off);

  memcpy(dst, fifo->data + off, l);
  memcmp(dst + l, fifo->data, len - l);
}

bool FIFO_Alloc(FIFO_t *fifo, uint32_t size, size_t ele_size) {
  ASSERT(size >= 2);

  fifo->in = 0;
  fifo->out = 0;
  fifo->ele_size = ele_size;

  fifo->data = pvPortMalloc(size * ele_size);

  if (!fifo->data) {
    fifo->mask = 0;
    return false;
  }
  fifo->mask = size - 1;
  return true;
}

bool FIFO_Free(FIFO_t *fifo) {
  vPortFree(fifo->data);
  fifo->in = 0;
  fifo->out = 0;
  fifo->ele_size = 0;
  fifo->data = NULL;
  fifo->mask = 0;
}

size_t FIFO_In(FIFO_t *fifo, const void *buf, size_t len) {
  uint32_t l = FIFO_Unused(fifo);

  if (len > l) len = l;

  FIFO_CopyIn(fifo, buf, len, fifo->in);
  fifo->in += len;
  return len;
}

size_t FIFO_Out(FIFO_t *fifo, void *buf, size_t len) {
  uint32_t l = fifo->in - fifo->out;

  if (len > l) len = l;
  FIFO_CopyOut(fifo, buf, len, fifo->out);
  fifo->out += len;
  return len;
}
