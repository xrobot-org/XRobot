#include "dev_tof.hpp"

#include <stdbool.h>
#include <string.h>

#define TOF_RES (1000) /* TOF数据分辨率 */

using namespace Device;

Tof::Tof(Param &param) : param_(param), recv_(sizeof(CAN::Pack), 1) {
  auto rx_callback = [](CAN::Pack &rx, Tof *tof) {
    rx.index -= tof->param_.index;
    if (rx.index < DEV_TOF_SENSOR_NUMBER) {
      tof->recv_.OverwriteFromISR(&rx);
    }

    return true;
  };

  auto tof_tp = Message::Topic<CAN::Pack>("can_tof");

  tof_tp.RegisterCallback(rx_callback, this);

  CAN::Subscribe(tof_tp, this->param_.can, this->param_.index,
                 DEV_TOF_SENSOR_NUMBER);

  auto tof_thread = [](void *arg) {
    Tof *tof = (Tof *)arg;

    while (1) {
      if (!tof->Update()) {
        tof->Offline();
      }

      tof->fb_tp_.Publish(tof->fb_);
      /* 运行结束，等待下一次唤醒 */
      tof->thread_.Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, tof_thread, 256, System::Thread::Realtime,
                 this);
}

void Tof::Decode(CAN::Pack &rx) {
  this->fb_[rx.index].dist =
      (float)((rx.data[2] << 16) | (rx.data[1] << 8) | rx.data[0]) /
      (float)TOF_RES;
  this->fb_[rx.index].status = rx.data[3];
  this->fb_[rx.index].signal_strength =
      (uint16_t)((rx.data[5] << 8) | rx.data[4]);
}

bool Tof::Update() {
  CAN::Pack pack;

  while (this->recv_.Receive(&pack, 2)) {
    this->Decode(pack);
  }

  return true;
}

void Tof::Offline() { memset(this->fb_, 0, sizeof(this->fb_)); }
