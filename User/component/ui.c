/*
  UI相关命令
*/
#include "component\ui.h"

#include <stdio.h>

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
                   uint16_t y_end) {
  if (grapic_line == NULL) return -1;
  snprintf((char *)grapic_line->name, 2, "%s", name);
  grapic_line->layer = layer;
  grapic_line->type_op = type_op;
  grapic_line->type_ele = 0;
  grapic_line->color = color;
  grapic_line->width = width;
  grapic_line->x_start = x_start;
  grapic_line->y_start = y_start;
  grapic_line->x_end = x_end;
  grapic_line->y_end = y_end;
  return 0;
}

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
                        uint16_t x_end, uint16_t y_end) {
  if (grapic_rectangle == NULL) return -1;
  snprintf((char *)grapic_rectangle->name, 2, "%s", name);
  grapic_rectangle->type_op = type_op;
  grapic_rectangle->type_ele = 1;
  grapic_rectangle->layer = layer;
  grapic_rectangle->color = color;
  grapic_rectangle->width = width;
  grapic_rectangle->x_start = x_start;
  grapic_rectangle->y_start = y_start;
  grapic_rectangle->x_end = x_end;
  grapic_rectangle->y_end = y_end;
  return 0;
}

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
                    uint16_t x_center, uint16_t y_center, uint16_t radius) {
  if (grapic_cycle == NULL) return -1;
  snprintf((char *)grapic_cycle->name, 2, "%s", name);
  grapic_cycle->type_op = type_op;
  grapic_cycle->layer = layer;
  grapic_cycle->type_ele = 2;
  grapic_cycle->color = color;
  grapic_cycle->width = width;
  grapic_cycle->x_start = x_center;
  grapic_cycle->y_start = y_center;
  grapic_cycle->radius = radius;
  return 0;
}

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
                   uint16_t y_semiaxis) {
  if (grapic_oval == NULL) return -1;
  snprintf((char *)grapic_oval->name, 2, "%s", name);
  grapic_oval->type_op = type_op;
  grapic_oval->type_ele = 3;
  grapic_oval->layer = layer;
  grapic_oval->color = color;
  grapic_oval->width = width;
  grapic_oval->x_start = x_center;
  grapic_oval->y_start = y_center;
  grapic_oval->x_end = x_semiaxis;
  grapic_oval->y_end = y_semiaxis;
  return 0;
}

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
                  uint16_t y_center, uint16_t x_semiaxis, uint16_t y_semiaxis) {
  if (grapic_arc == NULL) return -1;
  snprintf((char *)grapic_arc->name, 2, "%s", name);
  grapic_arc->type_op = type_op;
  grapic_arc->type_ele = 4;
  grapic_arc->layer = layer;
  grapic_arc->color = color;
  grapic_arc->angle_start = angle_start;
  grapic_arc->angle_end = angle_end;
  grapic_arc->width = width;
  grapic_arc->x_start = x_center;
  grapic_arc->y_start = y_center;
  grapic_arc->x_end = x_semiaxis;
  grapic_arc->y_end = y_semiaxis;
  return 0;
}

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
                       uint16_t float_middle, uint16_t float_low) {
  if (grapic_floating == NULL) return -1;
  snprintf((char *)grapic_floating->name, 2, "%s", name);
  grapic_floating->type_op = type_op;
  grapic_floating->type_ele = 5;
  grapic_floating->layer = layer;
  grapic_floating->color = color;
  grapic_floating->angle_start = font_size;
  grapic_floating->angle_end = digits;
  grapic_floating->width = width;
  grapic_floating->x_start = x_start;
  grapic_floating->y_start = y_start;
  grapic_floating->radius = float_high;
  grapic_floating->x_end = float_middle;
  grapic_floating->y_end = float_low;
  return 0;
}

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
                      uint16_t int32_t_middle, uint16_t int32_t_low) {
  if (grapic_integer == NULL) return -1;
  snprintf((char *)grapic_integer->name, 2, "%s", name);
  grapic_integer->type_op = type_op;
  grapic_integer->type_ele = 6;
  grapic_integer->layer = layer;
  grapic_integer->color = color;
  grapic_integer->angle_start = font_size;
  grapic_integer->width = width;
  grapic_integer->x_start = x_start;
  grapic_integer->y_start = y_start;
  grapic_integer->radius = int32_t_high;
  grapic_integer->x_end = int32_t_middle;
  grapic_integer->y_end = int32_t_low;
  return 0;
}

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
                        const char *character) {
  if (grapic_character == NULL) return -1;
  snprintf((char *)grapic_character->grapic.name, 2, "%s", name);
  grapic_character->grapic.type_op = type_op;
  grapic_character->grapic.type_ele = 7;
  grapic_character->grapic.layer = layer;
  grapic_character->grapic.color = color;
  grapic_character->grapic.angle_start = font_size;
  grapic_character->grapic.angle_end = length;
  grapic_character->grapic.width = width;
  grapic_character->grapic.x_start = x_start;
  grapic_character->grapic.y_start = y_start;
  snprintf((char *)grapic_character->character, 29, "%s", character);
  return 0;
}

/**
 * @brief UI_删除图层
 *
 * @param del 结构体
 * @param opt 操作
 * @param layer 图层
 * @return int8_t
 */
int8_t UI_DelLayer(UI_Del_t *del, uint8_t opt, uint8_t layer) {
  if (del == NULL) return -1;
  del->del_operation = opt;
  del->layer = layer;
  return 0;
}