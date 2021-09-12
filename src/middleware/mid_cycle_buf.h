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
} CycleBuf_t;

bool CycleBuf_Alloc(CycleBuf_t *cbuf, uint32_t size, size_t ele_size);
bool CycleBuf_Free(CycleBuf_t *cbuf);
size_t CycleBuf_In(CycleBuf_t *cbuf, const void *buf, size_t len);
size_t CycleBuf_Out(CycleBuf_t *cbuf, void *buf, size_t len);