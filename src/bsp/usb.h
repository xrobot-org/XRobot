#pragma once

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>
#include <stdint.h>
#include <string.h>

#include "bsp/bsp.h"

/* Exported constants ------------------------------------------------------- */
#define BSP_USB_MAX_RX_LEN 128  /* USB接收数据缓冲区长度 */
#define BSP_USB_MAX_TX_LEN 1024 /* USB发送数据缓冲区长度 */

/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

extern osThreadId_t gbsp_usb_alert; /* 暴露给USB库用以提示任务收到新内容 */
extern uint8_t usb_rx_buf[BSP_USB_MAX_RX_LEN]; /* 暴露给USB库用以存储接收数据 */
extern uint8_t usb_tx_buf[BSP_USB_MAX_TX_LEN]; /* 暴露给USB库用以存储发送数据 */
/* Exported functions prototypes -------------------------------------------- */
int8_t BSP_USB_ReadyReceive(osThreadId_t alert); /* 指示USB库准备好接收数据 */
char BSP_USB_ReadChar(void); /* 读取缓存第一个字符 */

int8_t BSP_USB_Printf(const char *fmt, ...); /* 打印至虚拟串口 */
