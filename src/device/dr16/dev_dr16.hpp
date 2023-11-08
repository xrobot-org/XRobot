#pragma once

#include <device.hpp>

#include "comp_cmd.hpp"
#include "comp_ui.hpp"

namespace Device {
class DR16 {
 public:
  /* 控制方式选择 */
  typedef enum {
    DR16_CTRL_SOURCE_SW,
    DR16_CTRL_SOURCE_MOUSE,
  } ControlSource;

  /* 拨杆开关 */
  typedef enum {
    DR16_SW_L_POS_TOP = 0,
    DR16_SW_L_POS_BOT,
    DR16_SW_L_POS_MID,
    DR16_SW_R_POS_TOP,
    DR16_SW_R_POS_BOT,
    DR16_SW_R_POS_MID,
    DR16_SW_POS_NUM
  } SwitchPos;

  /* 键盘按键 */
  typedef enum {
    KEY_W = DR16_SW_POS_NUM,
    KEY_S,
    KEY_A,
    KEY_D,
    KEY_SHIFT,
    KEY_CTRL,
    KEY_Q,
    KEY_E,
    KEY_R,
    KEY_F,
    KEY_G,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_L_PRESS,
    KEY_R_PRESS,
    KEY_L_RELEASE,
    KEY_R_RELEASE,
    KEY_NUM,
  } Key;

  constexpr uint32_t ShiftWith(Key key) { return key + 1 * KEY_NUM; }
  constexpr uint32_t CtrlWith(Key key) { return key + 2 * KEY_NUM; }
  constexpr uint32_t ShiftCtrlWith(Key key) { return key + 3 * KEY_NUM; }

  constexpr uint32_t RawValue(Key key) { return 1 << (key - KEY_W); }

  typedef struct __attribute__((packed)) {
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

  static void DrawUIStatic(DR16* dr16);

  static void DrawUIDynamic(DR16* dr16);

  static DR16::Data data_;

 private:
  Data last_data_{};

  ControlSource ctrl_source_ = DR16_CTRL_SOURCE_SW;

  System::Thread thread_;

  Message::Event event_;

  Message::Topic<Component::CMD::Data> cmd_tp_;

  Component::CMD::Data cmd_{};

  Component::UI::String string_{};

  Component::UI::Rectangle rectangle_{};
};
}  // namespace Device
