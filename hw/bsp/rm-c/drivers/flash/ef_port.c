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

#include "stm32f4xx_hal.h"

/* base address of the flash sectors */
#define ADDR_FLASH_SECTOR_0 \
  ((uint32_t)0x08000000) /* Base address of Sector 0, 16 K bytes   */
#define ADDR_FLASH_SECTOR_1 \
  ((uint32_t)0x08004000) /* Base address of Sector 1, 16 K bytes   */
#define ADDR_FLASH_SECTOR_2 \
  ((uint32_t)0x08008000) /* Base address of Sector 2, 16 K bytes   */
#define ADDR_FLASH_SECTOR_3 \
  ((uint32_t)0x0800C000) /* Base address of Sector 3, 16 K bytes   */
#define ADDR_FLASH_SECTOR_4 \
  ((uint32_t)0x08010000) /* Base address of Sector 4, 64 K bytes   */
#define ADDR_FLASH_SECTOR_5 \
  ((uint32_t)0x08020000) /* Base address of Sector 5, 128 K bytes  */
#define ADDR_FLASH_SECTOR_6 \
  ((uint32_t)0x08040000) /* Base address of Sector 6, 128 K bytes  */
#define ADDR_FLASH_SECTOR_7 \
  ((uint32_t)0x08060000) /* Base address of Sector 7, 128 K bytes  */
#define ADDR_FLASH_SECTOR_8 \
  ((uint32_t)0x08080000) /* Base address of Sector 8, 128 K bytes  */
#define ADDR_FLASH_SECTOR_9 \
  ((uint32_t)0x080A0000) /* Base address of Sector 9, 128 K bytes  */
#define ADDR_FLASH_SECTOR_10 \
  ((uint32_t)0x080C0000) /* Base address of Sector 10, 128 K bytes */
#define ADDR_FLASH_SECTOR_11 \
  ((uint32_t)0x080E0000) /* Base address of Sector 11, 128 K bytes */

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

static uint32_t stm32_get_sector(uint32_t address) {
  uint32_t sector = 0;

  if ((address < ADDR_FLASH_SECTOR_1) && (address >= ADDR_FLASH_SECTOR_0)) {
    sector = FLASH_SECTOR_0;
  } else if ((address < ADDR_FLASH_SECTOR_2) &&
             (address >= ADDR_FLASH_SECTOR_1)) {
    sector = FLASH_SECTOR_1;
  } else if ((address < ADDR_FLASH_SECTOR_3) &&
             (address >= ADDR_FLASH_SECTOR_2)) {
    sector = FLASH_SECTOR_2;
  } else if ((address < ADDR_FLASH_SECTOR_4) &&
             (address >= ADDR_FLASH_SECTOR_3)) {
    sector = FLASH_SECTOR_3;
  } else if ((address < ADDR_FLASH_SECTOR_5) &&
             (address >= ADDR_FLASH_SECTOR_4)) {
    sector = FLASH_SECTOR_4;
  } else if ((address < ADDR_FLASH_SECTOR_6) &&
             (address >= ADDR_FLASH_SECTOR_5)) {
    sector = FLASH_SECTOR_5;
  } else if ((address < ADDR_FLASH_SECTOR_7) &&
             (address >= ADDR_FLASH_SECTOR_6)) {
    sector = FLASH_SECTOR_6;
  } else if ((address < ADDR_FLASH_SECTOR_8) &&
             (address >= ADDR_FLASH_SECTOR_7)) {
    sector = FLASH_SECTOR_7;
  } else if ((address < ADDR_FLASH_SECTOR_9) &&
             (address >= ADDR_FLASH_SECTOR_8)) {
    sector = FLASH_SECTOR_8;
  } else if ((address < ADDR_FLASH_SECTOR_10) &&
             (address >= ADDR_FLASH_SECTOR_9)) {
    sector = FLASH_SECTOR_9;
  } else if ((address < ADDR_FLASH_SECTOR_11) &&
             (address >= ADDR_FLASH_SECTOR_10)) {
    sector = FLASH_SECTOR_10;
  } else {
    sector = FLASH_SECTOR_11;
  }
  return sector;
}

/**
 * Get the sector size
 *
 * @param sector sector
 *
 * @return sector size
 */
static uint32_t stm32_get_sector_size(uint32_t sector) {
  EF_ASSERT(IS_FLASH_SECTOR(sector));

  switch (sector) {
    case ADDR_FLASH_SECTOR_0:
      return 16 * 1024;
    case ADDR_FLASH_SECTOR_1:
      return 16 * 1024;
    case ADDR_FLASH_SECTOR_2:
      return 16 * 1024;
    case ADDR_FLASH_SECTOR_3:
      return 16 * 1024;
    case ADDR_FLASH_SECTOR_4:
      return 64 * 1024;
    case ADDR_FLASH_SECTOR_5:
      return 128 * 1024;
    case ADDR_FLASH_SECTOR_6:
      return 128 * 1024;
    case ADDR_FLASH_SECTOR_7:
      return 128 * 1024;
    case ADDR_FLASH_SECTOR_8:
      return 128 * 1024;
    case ADDR_FLASH_SECTOR_9:
      return 128 * 1024;
    case ADDR_FLASH_SECTOR_10:
      return 128 * 1024;
    case ADDR_FLASH_SECTOR_11:
      return 128 * 1024;
    default:
      return 128 * 1024;
  }
}

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

  FLASH_EraseInitTypeDef flash_erase;

  size_t erased_size = 0;

  uint32_t sector_error;

  /* make sure the start address is a multiple of EF_ERASE_MIN_SIZE */
  EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);

  HAL_FLASH_Unlock();

  /* You can add your code under here. */
  while (erased_size < size) {
    flash_erase.Sector = stm32_get_sector(addr + erased_size);
    flash_erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    flash_erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    flash_erase.NbSectors = 1;

    /* Erase FLASH*/
    HAL_FLASHEx_Erase(&flash_erase, &sector_error);

    erased_size += stm32_get_sector_size(flash_erase.Sector);
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

  /* You can add your code under here. */
  size_t i;
  uint8_t read_data;
  uint8_t *byte_buff = (uint8_t *)buf;

  HAL_FLASH_Unlock();

  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                         FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR |
                         FLASH_FLAG_PGSERR);

  for (i = 0; i < size; i += 1, byte_buff++, addr += 1) {
    /* write data */
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr, (uint64_t)*byte_buff);
    read_data = *(uint8_t *)byte_buff;
    /* check data */
    if (read_data != *byte_buff) {
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
