#include "bsp_uart.h"

#include <assert.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "bsp.h"

static bsp_callback_t callback_list[BSP_UART_NUM][BSP_UART_CB_NUM];

static int uart_fd[BSP_UART_NUM];

static uint32_t rx_count[BSP_UART_NUM];

static char uart_dev_path[] = "/dev/ttyCH9344USB0";

void bsp_uart_init() {
  for (int i = 0; i < BSP_UART_NUM; i++) {
    uart_dev_path[17] = '0' + i;
    uart_fd[i] = open(uart_dev_path, O_RDWR | O_NOCTTY);
    printf("uart %s dev id:%d\n", uart_dev_path, uart_fd[i]);
    assert(uart_fd[i] != -1);
    struct termios tty_cfg;

    tcgetattr(uart_fd[i], &tty_cfg);

    tty_cfg.c_cflag &= ~PARENB;
    tty_cfg.c_iflag &= ~INPCK;
    tty_cfg.c_cflag &= ~CSTOPB;
    tty_cfg.c_cflag &= ~CRTSCTS;

    cfsetispeed(&tty_cfg, B3500000);
    cfsetospeed(&tty_cfg, B3500000);

    // 一般必设置的标志
    tty_cfg.c_cflag |= (CLOCAL | CREAD);
    tty_cfg.c_oflag &= ~(OPOST);
    tty_cfg.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tty_cfg.c_iflag &= ~(ICRNL | INLCR | IGNCR | IXON | IXOFF | IXANY);

    tty_cfg.c_cc[VMIN] = 16;

    // 清空输入输出缓冲区
    tcflush(uart_fd[i], TCIOFLUSH);

    tty_cfg.c_cflag &= ~CSIZE;
    tty_cfg.c_cflag |= CS8;

    tcsetattr(uart_fd[i], TCSANOW, &tty_cfg);
  }
}

int8_t bsp_uart_register_callback(bsp_uart_t uart, bsp_uart_callback_t type,
                                  void (*callback)(void *),
                                  void *callback_arg) {
  assert(callback);
  assert(type != BSP_UART_CB_NUM);

  callback_list[uart][type].fn = callback;
  callback_list[uart][type].arg = callback_arg;
  return BSP_OK;
}

int8_t bsp_uart_transmit(bsp_uart_t uart, uint8_t *data, size_t size,
                         bool block) {
  if (block) {
    write(uart_fd[uart], data, size);
    return true;
  } else {
    assert(false);
    return BSP_ERR;
  }
}

int8_t bsp_uart_receive(bsp_uart_t uart, uint8_t *buff, size_t size,
                        bool block) {
  if (block) {
    rx_count[uart] = read(uart_fd[uart], buff, size);
    return true;
  } else {
    assert(false);
    return BSP_ERR;
  }
}

uint32_t bsp_uart_get_count(bsp_uart_t uart) { return rx_count[uart]; }

int8_t bsp_uart_abort_receive(bsp_uart_t uart) {
  tcflush(uart_fd[uart], TCIFLUSH);
  return BSP_OK;
}