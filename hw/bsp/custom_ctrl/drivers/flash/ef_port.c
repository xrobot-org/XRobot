/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015-2019, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-01-16
 */

#include <easyflash.h>
#include <stdarg.h>

#include "stm32f1xx_hal.h"

/* default environment variables set for user */
static const ef_env default_env_set[] = {{"runtime_rule", "\0"}};

/**
 * Flash port for hardware initialize.
 *
 * @param default_env default ENV set for user
 * @param default_env_size default ENV size
 *
 * @return result
 */
EfErrCode ef_port_init(ef_env const **default_env, size_t *default_env_size) {
  EfErrCode result = EF_NO_ERR;

  *default_env = default_env_set;
  *default_env_size = sizeof(default_env_set) / sizeof(default_env_set[0]);

  return result;
}

/**
 * Read data from flash.
 * @note This operation's units is word.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, size_t size) {
  EfErrCode result = EF_NO_ERR;

  /* You can add your code under here. */
  uint8_t *buf_8 = (uint8_t *)buf;
  size_t i;
  for (i = 0; i < size; i++, addr++, buf_8++) {
    *buf_8 = *(uint8_t *)addr;
  }
  return result;
}

/**
 * Erase data on flash.
 * @note This operation is irreversible.
 * @note This operation's units is different which on many chips.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
EfErrCode ef_port_erase(uint32_t addr, size_t size) {
  EfErrCode result = EF_NO_ERR;

  /* make sure the start address is a multiple of EF_ERASE_MIN_SIZE */
  EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);

  /* You can add your code under here. */
  HAL_FLASH_Unlock();

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR |
                         FLASH_FLAG_WRPERR);

  /* Erase FLASH*/
  FLASH_EraseInitTypeDef FlashSet;
  FlashSet.TypeErase = FLASH_TYPEERASE_PAGES;
  FlashSet.PageAddress = addr;
  FlashSet.NbPages = (size + EF_ERASE_MIN_SIZE - 1) / EF_ERASE_MIN_SIZE;

  /*Set PageError and call the erase function*/
  uint32_t PageError = 0;
  if (HAL_FLASHEx_Erase(&FlashSet, &PageError) != HAL_OK) result = EF_ERASE_ERR;

  HAL_FLASH_Lock();
  return result;
}
/**
 * Write data to flash.
 * @note This operation's units is word.
 * @note This operation must after erase. @see flash_erase.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */
EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, size_t size) {
  EfErrCode result = EF_NO_ERR;

  /* You can add your code under here. */
  size_t i;
  uint32_t read_data;

  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR |
                         FLASH_FLAG_WRPERR);
  for (i = 0; i < size; i += 4, buf++, addr += 4) {
    /* write data */
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, (uint64_t)*buf);
    read_data = *(uint32_t *)addr;
    /* check data */
    if (read_data != *buf) {
      result = EF_WRITE_ERR;
      break;
    }
  }
  HAL_FLASH_Lock();
  return result;
}

/**
 * lock the ENV ram cache
 */
void ef_port_env_lock(void) { /* You can add your code under here. */
}

/**
 * unlock the ENV ram cache
 */
void ef_port_env_unlock(void) { /* You can add your code under here. */
}

/**
 * This function is print flash debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 *
 */
void ef_log_debug(const char *file, const long line, const char *format, ...) {
#ifdef PRINT_DEBUG

  va_list args;

  /* args point to the first variable parameter */
  va_start(args, format);

  /* You can add your code under here. */

  va_end(args);

#endif
}

/**
 * This function is print flash routine info.
 *
 * @param format output format
 * @param ... args
 */
void ef_log_info(const char *format, ...) {
  va_list args;

  /* args point to the first variable parameter */
  va_start(args, format);

  /* You can add your code under here. */

  va_end(args);
}
/**
 * This function is print flash non-package info.
 *
 * @param format output format
 * @param ... args
 */
void ef_print(const char *format, ...) {
  va_list args;

  /* args point to the first variable parameter */
  va_start(args, format);

  /* You can add your code under here. */

  va_end(args);
}
