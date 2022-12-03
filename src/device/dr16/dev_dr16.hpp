#pragma once

#include "comp_cmd.hpp"
#include "comp_utils.hpp"
#include "dev.hpp"

namespace Device {
class DR16 {
 public:
  /* 控制方式选择 */
  typedef enum {
    ControlSourceSW,
    ControlSourceMouse,
  } ControlSource;

  /* 拨杆开关 */
  typedef enum {
    SwitchPosLeftTop = 0,
    SwitchPosLeftBot,
    SwitchPosLeftMid,
    SwitchPosRightTop,
    SwitchPosRightBot,
    SwitchPosRightMid,
    SwitchPosNum
  } SwitchPos;

  /* 键盘按键 */
  typedef enum {
    KeyW = SwitchPosNum,
    KeyS,
    KeyA,
    KeyD,
    KeySHIFT,
    KeyCTRL,
    KeyQ,
    KeyE,
    KeyR,
    KeyF,
    KeyG,
    KeyZ,
    KeyX,
    KeyC,
    KeyV,
    KeyB,
    KeyLClick,
    KeyRClick,
    KeyNum,
  } Key;

  constexpr uint32_t ShiftWith(Key key) { return key + 1 * KeyNum; }
  constexpr uint32_t CtrlWith(Key key) { return key + 2 * KeyNum; }
  constexpr uint32_t ShiftCtrlWith(Key key) { return key + 3 * KeyNum; }

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


  /**
   * @brief 开始接收
   *
   * @return true 成功
   * @return false 失败
   */
  bool StartRecv();

  /**
   * @brief 控制选择
   *
   */
  void PraseRC();

  /**
   * @brief 离线处理
   *
   */
  void Offline();

  /**
   * @brief 数据包损坏
   *
   * @return true 损坏
   * @return false 完好
   */
  bool DataCorrupted();

  static DR16::Data data_;

  Data last_data_;

  ControlSource ctrl_source_ = ControlSourceSW;

  System::Semaphore new_;

  System::Thread thread_;

  Message::Event event_;

  Message::Topic<Component::CMD::Data> cmd_tp_;

  Component::CMD::Data cmd_;
};
}  // namespace Device
