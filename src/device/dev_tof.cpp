#include "dev_tof.hpp"

#include <stdbool.h>
#include <string.h>

#define TOF_RES (1000) /* TOF数据分辨率 */

using namespace Device;

Tof::Tof(Param &param) : param_(param), recv_(sizeof(can_rx_item_t), 1) {
  auto rx_callback = [](om_msg_t *msg, void *arg) {
    Tof *tof = static_cast<Tof *>(arg);
    can_rx_item_t *rx = (can_rx_item_t *)msg->buff;
    rx->index -= tof->param_.index;
    if (rx->index < DEV_TOF_SENSOR_NUMBER) {
      tof->recv_.OverwriteFromISR(rx);
    }

    return OM_OK;
  };

  DECLARE_TOPIC(tof_tp, "can_tof", false);

  MESSAGE_REGISTER_CALLBACK(tof_tp, rx_callback, this);

  bsp_can_register_subscriber(this->param_.can, tof_tp.GetHandle(),
                              this->param_.index, DEV_TOF_SENSOR_NUMBER);

  auto tof_thread = [](void *arg) {
    Tof *tof = (Tof *)arg;

    while (1) {
      if (!tof->Update()) {
        tof->Offline();
      }

      tof->fb_.Publish();
      /* 运行结束，等待下一次唤醒 */
      tof->thread_.Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, tof_thread, 256, System::Thread::Realtime,
                 this);
}

void Tof::Decode(can_rx_item_t &rx) {
  this->fb_.data_[rx.index].dist =
      (float)((rx.data[2] << 16) | (rx.data[1] << 8) | rx.data[0]) /
      (float)TOF_RES;
  this->fb_.data_[rx.index].status = rx.data[3];
  this->fb_.data_[rx.index].signal_strength =
      (uint16_t)((rx.data[5] << 8) | rx.data[4]);
}

bool Tof::Update() {
  can_rx_item_t pack;

  while (this->recv_.Receive(&pack, 2)) {
    this->Decode(pack);
  }

  return true;
}

void Tof::Offline() { memset(this->fb_.data_, 0, sizeof(this->fb_.data_)); }
