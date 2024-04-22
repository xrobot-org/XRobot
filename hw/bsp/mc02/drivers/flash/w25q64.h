#ifndef __W25Q64_H__
#define __W25Q64_H__

#include <string.h>

#include "main.h"

/*----------------------------------------------- 命名参数宏
 * -------------------------------------------*/

#define OSPI_W25Qxx_OK 0              // W25Qxx通信正常
#define W25Qxx_ERROR_INIT -1          // 初始化错误
#define W25Qxx_ERROR_WriteEnable -2   // 写使能错误
#define W25Qxx_ERROR_AUTOPOLLING -3   // 轮询等待错误，无响应
#define W25Qxx_ERROR_Erase -4         // 擦除错误
#define W25Qxx_ERROR_TRANSMIT -5      // 传输错误
#define W25Qxx_ERROR_MemoryMapped -6  // 内存映射模式错误

#define W25Qxx_CMD_EnableReset 0x66  // 使能复位
#define W25Qxx_CMD_ResetDevice 0x99  // 复位器件
#define W25Qxx_CMD_JedecID 0x9F      // JEDEC ID
#define W25Qxx_CMD_WriteEnable 0X06  // 写使能

#define W25Qxx_CMD_SectorErase 0x20  // 扇区擦除，4K字节， 参考擦除时间 45ms
#define W25Qxx_CMD_BlockErase_32K 0x52  // 块擦除，  32K字节，参考擦除时间 120ms
#define W25Qxx_CMD_BlockErase_64K 0xD8  // 块擦除，  64K字节，参考擦除时间 150ms
#define W25Qxx_CMD_ChipErase 0xC7  // 整片擦除，参考擦除时间 20S

#define W25Qxx_CMD_QuadInputPageProgram \
  0x32  // 1-1-4模式下(1线指令1线地址4线数据)，页编程指令，参考写入时间 0.4ms
#define W25Qxx_CMD_FastReadQuad_IO \
  0xEB  // 1-4-4模式下(1线指令4线地址4线数据)，快速读取指令

#define W25Qxx_CMD_ReadStatus_REG1 0X05  // 读状态寄存器1
#define W25Qxx_Status_REG1_BUSY \
  0x01  // 读状态寄存器1的第0位（只读），Busy标志位，当正在擦除/写入数据/写命令时会被置1
#define W25Qxx_Status_REG1_WEL \
  0x02  // 读状态寄存器1的第1位（只读），WEL写使能标志位，该标志位为1时，代表可以进行写操作

#define W25Qxx_PageSize 256        // 页大小，256字节
#define W25Qxx_FlashSize 0x800000  // W25Q64大小，8M字节
#define W25Qxx_FLASH_ID 0Xef4017   // W25Q64 JEDEC ID
#define W25Qxx_ChipErase_TIMEOUT_MAX \
  100000U  // 超时等待时间，W25Q64整片擦除所需最大时间是100S
#define W25Qxx_Mem_Addr 0x90000000  // 内存映射模式的地址

/*----------------------------------------------- 函数声明
 * ---------------------------------------------------*/

int8_t OSPI_W25Qxx_Init(void);      // W25Qxx初始化
uint32_t OSPI_W25Qxx_ReadID(void);  // 读取器件ID

int8_t OSPI_W25Qxx_MemoryMappedMode(void);  // 将OSPI设置为内存映射模式

int8_t OSPI_W25Qxx_SectorErase(
    uint32_t SectorAddress);  // 扇区擦除，4K字节， 参考擦除时间 45ms
int8_t OSPI_W25Qxx_BlockErase_32K(
    uint32_t SectorAddress);  // 块擦除，  32K字节，参考擦除时间 120ms
int8_t OSPI_W25Qxx_BlockErase_64K(
    uint32_t SectorAddress);  // 块擦除，  64K字节，参考擦除时间
                              // 150ms，实际建议使用64K擦除，擦除的时间最快
int8_t OSPI_W25Qxx_ChipErase(void);  // 整片擦除，参考擦除时间 20S

int8_t OSPI_W25Qxx_WritePage(uint8_t* pBuffer, uint32_t WriteAddr,
                             uint16_t NumByteToWrite);  // 按页写入，最大256字节
int8_t OSPI_W25Qxx_WriteBuffer(
    uint8_t* pBuffer, uint32_t WriteAddr,
    uint32_t Size);  // 写入数据，最大不能超过flash芯片的大小
int8_t OSPI_W25Qxx_ReadBuffer(
    uint8_t* pBuffer, uint32_t ReadAddr,
    uint32_t NumByteToRead);  // 读取数据，最大不能超过flash芯片的大小

#endif
