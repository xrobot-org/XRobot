#include "bsp_udp_server.h"

#include <assert.h>
#include <hv/hloop.h>

bsp_status_t bsp_udp_server_start(bsp_udp_server_t *udp) {
  hio_read(udp->io);
  hloop_run(udp->loop);
  hloop_free(&udp->loop);

  return BSP_OK;
}

bsp_status_t bsp_udp_server_init(bsp_udp_server_t *udp, int port) {
  udp->loop = hloop_new(0);
  udp->io = hloop_create_udp_server(udp->loop, "0.0.0.0", port);

  assert(udp->io != NULL);

  memset(udp->cb, 0, sizeof(udp->cb));

  hevent_set_userdata(udp->io, udp);

  return BSP_OK;
}

static void bsp_udp_rx_cb(hio_t *io, void *buf, int readbytes) {
  bsp_udp_server_t *udp = hevent_userdata(io);

  if (udp->cb[BSP_UDP_RX_CPLT_CB].fn != NULL) {
    udp->cb[BSP_UDP_RX_CPLT_CB].fn(udp->cb[BSP_UDP_RX_CPLT_CB].arg, buf,
                                   readbytes);
  }
}

static void bsp_udp_tx_cb(hio_t *io, const void *buf, int readbytes) {
  (void)buf;
  bsp_udp_server_t *udp = hevent_userdata(io);
  if (udp->cb[BSP_UDP_TX_CPLT_CB].fn != NULL) {
    udp->cb[BSP_UDP_TX_CPLT_CB].fn(udp->cb[BSP_UDP_TX_CPLT_CB].arg, NULL,
                                   readbytes);
  }
}

bsp_status_t bsp_udp_server_register_callback(
    bsp_udp_server_t *udp, bsp_udp_server_callback_t type,
    void (*callback)(void *, void *, uint32_t), void *callback_arg) {
  udp->cb[type].fn = callback;
  udp->cb[type].arg = callback_arg;

  if (type == BSP_UDP_RX_CPLT_CB) {
    hio_setcb_read(udp->io, bsp_udp_rx_cb);
  } else if (type == BSP_UDP_TX_CPLT_CB) {
    hio_setcb_write(udp->io, bsp_udp_tx_cb);
  } else {
    assert(false);
  }

  return BSP_OK;
}

bsp_status_t bsp_udp_server_transmit(bsp_udp_server_t *udp, const uint8_t *data,
                                     uint32_t size) {
  hio_write(udp->io, data, size);
  return BSP_OK;
}
