#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"
#include "hv/hloop.h"
#include "hv/hsocket.h"

typedef struct {
  void (*fn)(void* arg, void* data, uint32_t size);
  void* arg;
} udp_callback_t;

typedef enum {
  BSP_UDP_TX_CPLT_CB,
  BSP_UDP_RX_CPLT_CB,
  BSP_UDP_SERVER_CB_NUM
} bsp_udp_server_callback_t;

typedef struct {
  hloop_t* loop;
  hio_t* io;
  udp_callback_t cb[BSP_UDP_SERVER_CB_NUM];
} bsp_udp_server_t;

bsp_status_t bsp_udp_server_init(bsp_udp_server_t* udp, int port);

bsp_status_t bsp_udp_server_start(bsp_udp_server_t* udp);

bsp_status_t bsp_udp_server_register_callback(
    bsp_udp_server_t* udp, bsp_udp_server_callback_t type,
    void (*callback)(void*, void*, uint32_t), void* callback_arg);

bsp_status_t bsp_udp_server_transmit(bsp_udp_server_t* udp, const uint8_t* data,
                                     uint32_t size);

#ifdef __cplusplus
}
#endif
