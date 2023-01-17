/*
  UI相关命令
*/

#pragma once

#include <component.hpp>

// NOLINTBEGIN(modernize-avoid-c-arrays)

#define UI_GRAPHIC_LAYER_CONST (0)
#define UI_GRAPHIC_LAYER_AUTOAIM (1)
#define UI_GRAPHIC_LAYER_CHASSIS (2)
#define UI_GRAPHIC_LAYER_CAP (3)
#define UI_GRAPHIC_LAYER_GIMBAL (4)
#define UI_GRAPHIC_LAYER_LAUNCHER (5)
#define UI_GRAPHIC_LAYER_CMD (6)

#define UI_DEFAULT_WIDTH (0x01)
#define UI_CHAR_DEFAULT_WIDTH (0x02)

#define UI_MAX_GRAPHIC_NUM (7)
#define UI_MAX_STRING_NUM (7)
#define UI_MAX_DEL_NUM (3)

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
} ui_color_t;

typedef enum {
  UI_DEL_OP_NOTHING = 0,
  UI_DEL_OP_DEL = 1,
  UI_DEL_OP_DEL_ALL = 2,
} ui_del_op_t;

typedef enum {
  UI_GRAPHIC_OP_NOTHING = 0,
  UI_GRAPHIC_OP_ADD = 1,
  UI_GRAPHIC_OP_REWRITE = 2,
  UI_GRAPHIC_OP_DEL = 3,
} ui_graphic_op_t;

typedef struct __attribute__((packed)) {
  uint8_t name[3];  // NOLINT(modernize-avoid-c-arrays)
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
} ui_ele_t;

typedef struct __attribute__((packed)) {
  ui_ele_t graphic;
  uint8_t str[30];  // NOLINT(modernize-avoid-c-arrays)
} ui_string_t;

typedef struct __attribute__((packed)) {
  uint8_t op;
  uint8_t layer;
} ui_del_t;

typedef struct {
  uint16_t width;
  uint16_t height;
} ui_screen_t;

/**
 * @brief UI_绘制直线段
 *
 * @param ele 结构体
 * @param name 图形名首地址
 * @param op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param width 线条宽度
 * @param x_start 起点x坐标
 * @param y_start 起点y坐标
 * @param x_end 终点x坐标
 * @param y_end 终点y坐标
 * @return int8_t
 */
int8_t ui_draw_line(ui_ele_t *ele, const char *name, ui_graphic_op_t op,
                    uint8_t layer, ui_color_t color, uint16_t width,
                    uint16_t x_start, uint16_t y_start, uint16_t x_end,
                    uint16_t y_end);

/**
 * @brief UI_绘制矩形
 *
 * @param ele 结构体
 * @param name 图形名首地址
 * @param op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param width 线条宽度
 * @param x_start 起点x坐标
 * @param y_start 起点y坐标
 * @param x_end 对角顶点x坐标
 * @param y_end 对角顶点y坐标
 * @return int8_t
 */
int8_t ui_draw_rectangle(ui_ele_t *ele, const char *name, ui_graphic_op_t op,
                         uint8_t layer, ui_color_t color, uint16_t width,
                         uint16_t x_start, uint16_t y_start, uint16_t x_end,
                         uint16_t y_end);

/**
 * @brief UI_绘制正圆
 *
 * @param ele 结构体
 * @param name 图形名首地址
 * @param op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param width 线条宽度
 * @param x_center 圆心x坐标
 * @param y_center 圆心y坐标
 * @param radius 半径
 * @return int8_t
 */
int8_t ui_draw_cycle(ui_ele_t *ele, const char *name, ui_graphic_op_t op,
                     uint8_t layer, ui_color_t color, uint16_t width,
                     uint16_t x_center, uint16_t y_center, uint16_t radius);

/**
 * @brief UI_绘制椭圆
 *
 * @param ele 结构体
 * @param name 图形名首地址
 * @param op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param width 线条宽度
 * @param x_center 圆心x坐标
 * @param y_center 圆心y坐标
 * @param x_semiaxis x半轴长度
 * @param y_semiaxis y半轴长度
 * @return int8_t
 */
int8_t ui_draw_oval(ui_ele_t *ele, const char *name, ui_graphic_op_t op,
                    uint8_t layer, ui_color_t color, uint16_t width,
                    uint16_t x_center, uint16_t y_center, uint16_t x_semiaxis,
                    uint16_t y_semiaxis);

/**
 * @brief UI_绘制圆弧
 *
 * @param ele 结构体
 * @param name 图形名首地址
 * @param op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param angle_start 起始角度
 * @param angle_end 终止角度
 * @param width 线条宽度
 * @param x_center 圆心x坐标
 * @param y_center 圆心y坐标
 * @param x_semiaxis x半轴长度
 * @param y_semiaxis y半轴长度
 * @return int8_t
 */
int8_t ui_draw_arc(ui_ele_t *ele, const char *name, ui_graphic_op_t op,
                   uint8_t layer, ui_color_t color, uint16_t angle_start,
                   uint16_t angle_end, uint16_t width, uint16_t x_center,
                   uint16_t y_center, uint16_t x_semiaxis, uint16_t y_semiaxis);

/**
 * @brief UI_绘制浮点数
 *
 * @param ele 结构体
 * @param name 图形名首地址
 * @param op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param font_size 字体大小
 * @param digits 小数点后有效位数
 * @param width 线条宽度
 * @param x_start 起点x坐标
 * @param y_start 起点y坐标
 * @param float_high 32位浮点数
 * @param float_middle 32位浮点数
 * @param float_low 32位浮点数
 * @return int8_t
 */
int8_t ui_draw_float(ui_ele_t *ele, const char *name, ui_graphic_op_t op,
                     uint8_t layer, ui_color_t color, uint16_t font_size,
                     uint16_t digits, uint16_t width, uint16_t x_start,
                     uint16_t y_start, uint16_t float_high,
                     uint16_t float_middle, uint16_t float_low);

/**
 * @brief UI_绘制整型数
 *
 * @param ele 结构体
 * @param name 图形名首地址
 * @param op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param font_size 字体大小
 * @param width 线条宽度
 * @param x_start 起点x坐标
 * @param y_start 起点y坐标
 * @param int32_t_high 32位整型数
 * @param int32_t_middle 32位整型数
 * @param int32_t_low 32位整型数
 * @return int8_t
 */
int8_t ui_draw_int(ui_ele_t *ele, const char *name, ui_graphic_op_t op,
                   uint8_t layer, ui_color_t color, uint16_t font_size,
                   uint16_t width, uint16_t x_start, uint16_t y_start,
                   uint16_t int32_t_high, uint16_t int32_t_middle,
                   uint16_t int32_t_low);

/**
 * @brief UI_绘制字符
 *
 * @param ele 结构体
 * @param name 图形名首地址
 * @param op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param font_size 字体大小
 * @param length 字符长度
 * @param width 线条宽度
 * @param x_start 起点x坐标
 * @param y_start 起点y坐标
 * @param str 字符串首地址
 * @return int8_t
 */
int8_t ui_draw_string(ui_string_t *ele, const char *name, ui_graphic_op_t op,
                      uint8_t layer, ui_color_t color, uint16_t font_size,
                      uint16_t length, uint16_t width, uint16_t x_start,
                      uint16_t y_start, const char *str);

/**
 * @brief UI_删除图层
 *
 * @param del 结构体
 * @param op 操作
 * @param layer 图层
 * @return int8_t
 */
int8_t ui_del_layer(ui_del_t *del, ui_del_op_t op, uint8_t layer);

// NOLINTEND(modernize-avoid-c-arrays)
