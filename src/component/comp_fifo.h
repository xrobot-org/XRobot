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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 循环缓冲区 */
typedef struct {
  uint32_t in;
  uint32_t out;
  uint32_t mask;
  uint32_t size;
  size_t ele_size;
  void *data;
} FIFO_t;

bool FIFO_Alloc(FIFO_t *fifo, uint32_t size, size_t ele_size);
bool FIFO_Free(FIFO_t *fifo);
size_t FIFO_In(FIFO_t *fifo, const void *buf, size_t len);
size_t FIFO_Out(FIFO_t *fifo, void *buf, size_t len);