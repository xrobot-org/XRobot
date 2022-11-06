/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
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
 * Function: Portable interface for stm32f10x platform.
 * Created on: 2015-01-16
 */

#include <easyflash.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* default environment variables set for user */
static const ef_env default_env_set[] = {{
                                             "SN",
                                             "\0",
                                             32 * sizeof(uint8_t),
                                         },
                                         {
                                             "WRITE TIMES",
                                             "\0",
                                             sizeof(uint32_t),
                                         }};

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
  uint8_t *buf_8 = (uint8_t *)buf;
  size_t i;

  /*copy from flash to ram */
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

  FLASH_EraseInitTypeDef flash_erase;

  size_t erased_size = 0;

  uint32_t sector_error;

  /* make sure the start address is a multiple of EF_ERASE_MIN_SIZE */
  EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);

  HAL_FLASH_Unlock();

  /* You can add your code under here. */
  while (erased_size < size) {
    flash_erase.NbPages = 1;
    flash_erase.PageAddress = addr;
    flash_erase.PageAddress - ((addr - FLASH_BASE) % EF_ERASE_MIN_SIZE);
    flash_erase.TypeErase = FLASH_TYPEERASE_PAGES;

    /* Erase FLASH*/
    HAL_FLASHEx_Erase(&flash_erase, &sector_error);

    erased_size += EF_ERASE_MIN_SIZE;
  }

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
  size_t i;
  uint32_t read_data;

  HAL_FLASH_Unlock();
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR);
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

void ef_log_info(const char *format, ...) { (void)format; }

void ef_print(const char *format, ...) { (void)format; }
