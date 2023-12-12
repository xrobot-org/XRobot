#include "bsp_uart.h"

#define termios asmtermios
#include <asm/termbits.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#undef termios
#include <termios.h>
#include <unistd.h>

#include "bsp.h"

static bsp_callback_t callback_list[BSP_UART_NUM][BSP_UART_CB_NUM];

static int uart_fd[BSP_UART_NUM];

static int rx_count[BSP_UART_NUM];

static bool uart_block[BSP_UART_NUM];

static const char *uart_dev_path[] = {"/dev/ttyCH343USB1", "/dev/ttyCH343USB0"};

static const uint32_t UART_SPEED[] = {9000000, 9000000};

static int libtty_setcustombaudrate(int fd, int baudrate) {
  struct termios2 tio;

  if (ioctl(fd, TCGETS2, &tio)) {
    perror("TCGETS2");
    return -1;
  }

  tio.c_cflag &= ~CBAUD;
  tio.c_cflag |= BOTHER;
  tio.c_ispeed = baudrate;
  tio.c_ospeed = baudrate;

  if (ioctl(fd, TCSETS2, &tio)) {
    perror("TCSETS2");
    return -1;
  }

  if (ioctl(fd, TCGETS2, &tio)) {
    perror("TCGETS2");
    return -1;
  }

  return 0;
}

static int libtty_open(const char *devname) {
  int fd = open(devname, O_RDWR | O_NOCTTY);
  int flags = 0;

  if (fd < 0) {
    perror("open device failed");
    return -1;
  }

  flags = fcntl(fd, F_GETFL, 0);
  flags &= ~O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) < 0) {
    printf("fcntl failed.\n");
    return -1;
  }

  if (isatty(fd) == 0) {
    printf("not tty device.\n");
    return -1;
  } else
    printf("tty device test ok.\n");

  return fd;
}

static int libtty_setopt(int fd, int speed, int databits, int stopbits,
                         char parity, char hardflow) {
  struct termios newtio;
  struct termios oldtio;

  bzero(&newtio, sizeof(newtio));
  bzero(&oldtio, sizeof(oldtio));

  if (tcgetattr(fd, &oldtio) != 0) {
    perror("tcgetattr");
    return -1;
  }
  newtio.c_cflag |= CLOCAL | CREAD;
  newtio.c_cflag &= ~CSIZE;

  /* set data bits */
  switch (databits) {
    case 5:
      newtio.c_cflag |= CS5;
      break;
    case 6:
      newtio.c_cflag |= CS6;
      break;
    case 7:
      newtio.c_cflag |= CS7;
      break;
    case 8:
      newtio.c_cflag |= CS8;
      break;
    default:
      fprintf(stderr, "unsupported data size\n");
      return -1;
  }

  /* set parity */
  switch (parity) {
    case 'n':
    case 'N':
      newtio.c_cflag &= ~PARENB; /* Clear parity enable */
      newtio.c_iflag &= ~INPCK;  /* Disable input parity check */
      break;
    case 'o':
    case 'O':
      newtio.c_cflag |= (PARODD | PARENB); /* Odd parity instead of even */
      newtio.c_iflag |= INPCK;             /* Enable input parity check */
      break;
    case 'e':
    case 'E':
      newtio.c_cflag |= PARENB;  /* Enable parity */
      newtio.c_cflag &= ~PARODD; /* Even parity instead of odd */
      newtio.c_iflag |= INPCK;   /* Enable input parity check */
      break;
    default:
      fprintf(stderr, "unsupported parity\n");
      return -1;
  }

  /* set stop bits */
  switch (stopbits) {
    case 1:
      newtio.c_cflag &= ~CSTOPB;
      break;
    case 2:
      newtio.c_cflag |= CSTOPB;
      break;
    default:
      perror("unsupported stop bits\n");
      return -1;
  }

  if (hardflow)
    newtio.c_cflag |= CRTSCTS;
  else
    newtio.c_cflag &= ~CRTSCTS;

  newtio.c_cc[VTIME] = 10; /* Time-out value (tenths of a second) [!ICANON]. */
  newtio.c_cc[VMIN] = 0;   /* Minimum number of bytes read at once [!ICANON]. */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) != 0) {
    perror("tcsetattr");
    return -1;
  }

  /* set tty speed */
  if (libtty_setcustombaudrate(fd, speed) != 0) {
    perror("setbaudrate");
    return -1;
  }

  return 0;
}

void bsp_uart_init() {
  for (int i = 0; i < BSP_UART_NUM; i++) {
    uart_fd[i] = libtty_open(uart_dev_path[i]);
    if (uart_fd[i] <= 0) {
      XB_ASSERT(false);
    }
    libtty_setopt(uart_fd[i], UART_SPEED[i], 8, 1, 'n', 0);
  }
}

bsp_status_t bsp_uart_register_callback(bsp_uart_t uart,
                                        bsp_uart_callback_t type,
                                        void (*callback)(void *),
                                        void *callback_arg) {
  assert(callback);
  assert(type != BSP_UART_CB_NUM);

  callback_list[uart][type].fn = callback;
  callback_list[uart][type].arg = callback_arg;
  return BSP_OK;
}

bsp_status_t bsp_uart_transmit(bsp_uart_t uart, uint8_t *data, size_t size,
                               bool block) {
  if (block) {
    write(uart_fd[uart], data, size);
    return true;
  } else {
    assert(false);
    return BSP_ERR;
  }
}

bsp_status_t bsp_uart_receive(bsp_uart_t uart, uint8_t *buff, size_t size,
                              bool block) {
  if (block && !uart_block[uart]) {
    fcntl(uart_fd[uart], F_SETFL, 0);
    uart_block[uart] = block;
  }
  if (!block && uart_block[uart]) {
    fcntl(uart_fd[uart], F_SETFL, FNDELAY);
    uart_block[uart] = block;
  }

rework:
  rx_count[uart] = read(uart_fd[uart], buff, size);

  while (rx_count[uart] != (int)(size)) {
    size -= rx_count[uart];
    buff += rx_count[uart];
    goto rework;
    return BSP_ERR;
  }

  return true;
}

uint32_t bsp_uart_get_count(bsp_uart_t uart) { return rx_count[uart]; }

bsp_status_t bsp_uart_abort_receive(bsp_uart_t uart) {
  tcflush(uart_fd[uart], TCIFLUSH);
  return BSP_OK;
}
