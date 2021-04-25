/*
  UI相关命令
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

#include "component\user_math.h"

#define UI_DEL_OPERATION_NOTHING (0)
#define UI_DEL_OPERATION_DEL (1)
#define UI_DEL_OPERATION_DEL_ALL (2)

#define UI_GRAPIC_OPERATION_NOTHING (0)
#define UI_GRAPIC_OPERATION_ADD (1)
#define UI_GRAPIC_OPERATION_REWRITE (2)
#define UI_GRAPIC_OPERATION_DEL (3)

#define UI_GRAPIC_LAYER_CONST (0)
#define UI_GRAPIC_LAYER_AUTOAIM (1)
#define UI_GRAPIC_LAYER_CHASSIS (2)
#define UI_GRAPIC_LAYER_CAP (3)
#define UI_GRAPIC_LAYER_GIMBAL (4)
#define UI_GRAPIC_LAYER_LAUNCHER (5)
#define UI_GRAPIC_LAYER_CMD (6)

#define UI_DEFAULT_WIDTH (0x01)
#define UI_CHAR_DEFAULT_WIDTH (0x02)

typedef enum {
  RED_BLUE,
  YELLOW,
  GREEN,
  ORANGE,
  PURPLISH_RED,
  PINK,
  CYAN,
  BLACK,
  WHITE
} UI_Color_t;

typedef struct __packed {
  uint8_t op;
  uint8_t num_layer;
} UI_InterStudent_UIDel_t;

typedef struct __packed {
  uint8_t name[3];
  uint8_t type_op : 3;
  uint8_t type_ele : 3;
  uint8_t layer : 4;
  uint8_t color : 4;
  uint16_t angle_start : 9;
  uint16_t angle_end : 9;
  uint16_t width : 10;
  uint16_t x_start : 11;
  uint16_t y_start : 11;
  uint16_t radius : 10;
  uint16_t x_end : 11;
  uint16_t y_end : 11;
} UI_Ele_t;

typedef struct __packed {
  UI_Ele_t grapic;
} UI_Drawgrapic_1_t;

typedef struct __packed {
  UI_Ele_t grapic[2];
} UI_Drawgrapic_2_t;

typedef struct __packed {
  UI_Ele_t grapic[5];
} UI_Drawgrapic_5_t;

typedef struct __packed {
  UI_Ele_t grapic[7];
} UI_Drawgrapic_7_t;

typedef struct __packed {
  UI_Ele_t grapic;
  uint8_t character[30];
} UI_Drawcharacter_t;

typedef struct __packed {
  uint8_t del_operation;
  uint8_t layer;
} UI_Del_t;

typedef struct {
  uint16_t width;
  uint16_t height;
} UI_Screen_t;

/**
 * @brief UI_绘制直线段
 *
 * @param grapic_line 结构体
 * @param name 图形名首地址
 * @param type_op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param width 线条宽度
 * @param x_start 起点x坐标
 * @param y_start 起点y坐标
 * @param x_end 终点x坐标
 * @param y_end 终点y坐标
 * @return int8_t
 */
int8_t UI_DrawLine(UI_Ele_t *grapic_line, const char *name, uint8_t type_op,
                   uint8_t layer, uint8_t color, uint16_t width,
                   uint16_t x_start, uint16_t y_start, uint16_t x_end,
                   uint16_t y_end);

/**
 * @brief UI_绘制矩形
 *
 * @param grapic_rectangle 结构体
 * @param name 图形名首地址
 * @param type_op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param width 线条宽度
 * @param x_start 起点x坐标
 * @param y_start 起点y坐标
 * @param x_end 对角顶点x坐标
 * @param y_end 对角顶点y坐标
 * @return int8_t
 */
int8_t UI_DrawRectangle(UI_Ele_t *grapic_rectangle, const char *name,
                        uint8_t type_op, uint8_t layer, uint8_t color,
                        uint16_t width, uint16_t x_start, uint16_t y_start,
                        uint16_t x_end, uint16_t y_end);

/**
 * @brief UI_绘制正圆
 *
 * @param grapic_cycle 结构体
 * @param name 图形名首地址
 * @param type_op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param width 线条宽度
 * @param x_center 圆心x坐标
 * @param y_center 圆心y坐标
 * @param radius 半径
 * @return int8_t
 */
int8_t UI_DrawCycle(UI_Ele_t *grapic_cycle, const char *name, uint8_t type_op,
                    uint8_t layer, uint8_t color, uint16_t width,
                    uint16_t x_center, uint16_t y_center, uint16_t radius);

/**
 * @brief UI_绘制椭圆
 *
 * @param grapic_oval 结构体
 * @param name 图形名首地址
 * @param type_op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param width 线条宽度
 * @param x_center 圆心x坐标
 * @param y_center 圆心y坐标
 * @param x_semiaxis x半轴长度
 * @param y_semiaxis y半轴长度
 * @return int8_t
 */
int8_t UI_DrawOval(UI_Ele_t *grapic_oval, const char *name, uint8_t type_op,
                   uint8_t layer, uint8_t color, uint16_t width,
                   uint16_t x_center, uint16_t y_center, uint16_t x_semiaxis,
                   uint16_t y_semiaxis);

/**
 * @brief UI_绘制圆弧
 *
 * @param grapic_arc 结构体
 * @param name 图形名首地址
 * @param type_op 操作类型
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
int8_t UI_DrawArc(UI_Ele_t *grapic_arc, const char *name, uint8_t type_op,
                  uint8_t layer, uint8_t color, uint16_t angle_start,
                  uint16_t angle_end, uint16_t width, uint16_t x_center,
                  uint16_t y_center, uint16_t x_semiaxis, uint16_t y_semiaxis);

/**
 * @brief UI_绘制浮点数
 *
 * @param grapic_float 结构体
 * @param name 图形名首地址
 * @param type_op 操作类型
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
int8_t UI_DrawFloating(UI_Ele_t *grapic_floating, const char *name,
                       uint8_t type_op, uint8_t layer, uint8_t color,
                       uint16_t font_size, uint16_t digits, uint16_t width,
                       uint16_t x_start, uint16_t y_start, uint16_t float_high,
                       uint16_t float_middle, uint16_t float_low);

/**
 * @brief UI_绘制整型数
 *
 * @param grapic_integer 结构体
 * @param name 图形名首地址
 * @param type_op 操作类型
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
int8_t UI_DrawInteger(UI_Ele_t *grapic_integer, const char *name,
                      uint8_t type_op, uint8_t layer, uint8_t color,
                      uint16_t font_size, uint16_t width, uint16_t x_start,
                      uint16_t y_start, uint16_t int32_t_high,
                      uint16_t int32_t_middle, uint16_t int32_t_low);

/**
 * @brief UI_绘制字符
 *
 * @param grapic_character 结构体
 * @param name 图形名首地址
 * @param type_op 操作类型
 * @param layer 图层数
 * @param color 颜色
 * @param font_size 字体大小
 * @param length 字符长度
 * @param width 线条宽度
 * @param x_start 起点x坐标
 * @param y_start 起点y坐标
 * @param character 字符串首地址
 * @return int8_t
 */
int8_t UI_DrawCharacter(UI_Drawcharacter_t *grapic_character, const char *name,
                        uint8_t type_op, uint8_t layer, uint8_t color,
                        uint16_t font_size, uint16_t length, uint16_t width,
                        uint16_t x_start, uint16_t y_start,
                        const char *character);

/**
 * @brief UI_删除图层
 *
 * @param del 结构体
 * @param opt 操作
 * @param layer 图层
 * @return int8_t
 */
int8_t UI_DelLayer(UI_Del_t *del, uint8_t opt, uint8_t layer);

#ifdef __cplusplus
}
#endif