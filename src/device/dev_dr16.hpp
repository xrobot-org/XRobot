#pragma once

#include "comp_cmd.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"

namespace Device {
class DR16 {
 public:
  typedef struct __packed {
    uint16_t ch_r_x : 11;
    uint16_t ch_r_y : 11;
    uint16_t ch_l_x : 11;
    uint16_t ch_l_y : 11;
    uint8_t sw_r : 2;
    uint8_t sw_l : 2;
    int16_t x;
    int16_t y;
    int16_t z;
    uint8_t press_l;
    uint8_t press_r;
    uint16_t key;
    uint16_t res;
  } Data;

  DR16();

  bool StartRecv();

  void PraseRC();

  void Offline();

  bool DataCorrupted();

  static Data data_;

  Component::CMD::RC rc_;

  System::Semaphore new_;

  System::Thread thread_;
};
}  // namespace Device
