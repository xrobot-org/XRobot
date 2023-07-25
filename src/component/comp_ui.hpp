/*
  UI相关命令
*/

#pragma once

#include <component.hpp>

#define UI_DEFAULT_WIDTH (0x01)
#define UI_CHAR_DEFAULT_WIDTH (0x02)

#define UI_MAX_GRAPHIC_NUM (7)
#define UI_MAX_STRING_NUM (7)
#define UI_MAX_DEL_NUM (3)

namespace Component {
class UI {
 public:
  typedef enum {
    UI_GRAPHIC_LAYER_CONST = 0,
    UI_GRAPHIC_LAYER_AUTOAIM,
    UI_GRAPHIC_LAYER_CHASSIS,
    UI_GRAPHIC_LAYER_CAP,
    UI_GRAPHIC_LAYER_GIMBAL,
    UI_GRAPHIC_LAYER_LAUNCHER,
    UI_GRAPHIC_LAYER_CMD
  } Layer;

  typedef enum {
    UI_RED_BLUE = 0,
    UI_YELLOW,
    UI_GREEN,
    UI_ORANGE,
    UI_PURPLISH_RED,
    UI_PINK,
    UI_CYAN,
    UI_BLACK,
    UI_WHITE
  } Color;

  typedef enum {
    UI_DEL_OP_NOTHING = 0,
    UI_DEL_OP_DEL = 1,
    UI_DEL_OP_DEL_ALL = 2,
  } DelOperation;

  typedef enum {
    UI_GRAPHIC_OP_NOTHING = 0,
    UI_GRAPHIC_OP_ADD = 1,
    UI_GRAPHIC_OP_REWRITE = 2,
    UI_GRAPHIC_OP_DEL = 3,
  } GraphicOperation;

  typedef struct __attribute__((packed)) {
    uint8_t name[3];
    uint16_t op : 3;
    uint16_t type_ele : 3;
    uint16_t layer : 4;
    uint16_t color : 4;
    uint16_t angle_start : 9;
    uint16_t angle_end : 9;
    uint16_t width : 10;
    uint16_t x_start : 11;
    uint16_t y_start : 11;
    uint16_t radius : 10;
    uint16_t x_end : 11;
    uint16_t y_end : 11;
  } Ele;
  typedef struct __attribute__((packed)) {
    Ele graphic;
    uint8_t str[30];
  } Str;
  typedef struct __attribute__((packed)) {
    uint8_t op;
    uint8_t layer;
  } Del;
  typedef struct {
    uint16_t width;
    uint16_t height;
  } Scr;
  class Line {
   public:
    int8_t Draw(const char *name, GraphicOperation op, uint8_t layer,
                Color color, uint16_t width, uint16_t x_start, uint16_t y_start,
                uint16_t x_end, uint16_t y_end);

    Ele ele_;

    operator Ele() { return ele_; }
  };

  class Rectangle {
   public:
    int8_t Draw(const char *name, GraphicOperation op, uint8_t layer,
                Color color, uint16_t width, uint16_t x_start, uint16_t y_start,
                uint16_t x_end, uint16_t y_end);

    Ele ele_;

    operator Ele() { return ele_; }
  };

  class Cycle {
   public:
    Ele ele_;

    int8_t Draw(const char *name, GraphicOperation op, uint8_t layer,
                Color color, uint16_t width, uint16_t x_center,
                uint16_t y_center, uint16_t radius);
    operator Ele() { return ele_; }
  };
  class Oval {
   public:
    Ele ele_;
    int8_t Draw(const char *name, GraphicOperation op, uint8_t layer,
                Color color, uint16_t width, uint16_t x_center,
                uint16_t y_center, uint16_t x_semiaxis, uint16_t y_semiaxis);
    operator Ele() { return ele_; }
  };

  class Arc {
   public:
    Ele ele_;

    int8_t Draw(const char *name, GraphicOperation op, uint8_t layer,
                Color color, uint16_t angle_start, uint16_t angle_end,
                uint16_t width, uint16_t x_center, uint16_t y_center,
                uint16_t x_semiaxis, uint16_t y_semiaxis);
    operator Ele() { return ele_; }
  };

  class FloatNum {
   public:
    Ele ele_;

    int8_t Draw(const char *name, GraphicOperation op, uint8_t layer,
                Color color, uint16_t font_size, uint16_t digits,
                uint16_t width, uint16_t x_start, uint16_t y_start,
                uint16_t float_high, uint16_t float_middle, uint16_t float_low);
    operator Ele() { return ele_; }
  };

  class IntNum {
   public:
    Ele ele_;

    int8_t Draw(const char *name, GraphicOperation op, uint8_t layer,
                Color color, uint16_t font_size, uint16_t width,
                uint16_t x_start, uint16_t y_start, uint16_t int32_t_high,
                uint16_t int32_t_middle, uint16_t int32_t_low);
    operator Ele() { return ele_; }
  };

  class String {
   public:
    Str str_;

    int8_t Draw(const char *name, GraphicOperation op, uint8_t layer,
                Color color, uint16_t font_size, uint16_t length,
                uint16_t width, uint16_t x_start, uint16_t y_start,
                const char *str);
    operator Str() { return str_; }
  };

  class Delete {
   public:
    Del del_;

    int8_t Draw(DelOperation op, uint8_t layer);
    operator Del() { return del_; }
  };
};
}  // namespace Component
