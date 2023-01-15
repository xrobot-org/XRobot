/*
  UI相关命令
*/
#include "comp_ui.hpp"

typedef enum {
  UI_ELE_LINE = 0,
  UI_ELE_RECT,
  UI_ELE_CYCLE,
  UI_ELE_OVAL,
  UI_ELE_ARC,
  UI_ELE_FLOAT,
  UI_ELE_INT,
  UI_ELE_STR,
} ui_ele_type_t;

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
                    uint16_t y_end) {
  ASSERT(ele);
  (void)snprintf(reinterpret_cast<char *>(ele->name), 2, "%-2s", name);
  ele->layer = layer;
  ele->op = op;
  ele->type_ele = UI_ELE_LINE;
  ele->color = color;
  ele->width = width;
  ele->x_start = x_start;
  ele->y_start = y_start;
  ele->x_end = x_end;
  ele->y_end = y_end;
  return 0;
}

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
                         uint16_t y_end) {
  ASSERT(ele);
  (void)snprintf(reinterpret_cast<char *>(ele->name), 2, "%-2s", name);
  ele->op = op;
  ele->type_ele = UI_ELE_RECT;
  ele->layer = layer;
  ele->color = color;
  ele->width = width;
  ele->x_start = x_start;
  ele->y_start = y_start;
  ele->x_end = x_end;
  ele->y_end = y_end;
  return 0;
}

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
                     uint16_t x_center, uint16_t y_center, uint16_t radius) {
  ASSERT(ele);
  (void)snprintf(reinterpret_cast<char *>(ele->name), 2, "%-2s", name);
  ele->op = op;
  ele->layer = layer;
  ele->type_ele = UI_ELE_CYCLE;
  ele->color = color;
  ele->width = width;
  ele->x_start = x_center;
  ele->y_start = y_center;
  ele->radius = radius;
  return 0;
}

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
                    uint16_t y_semiaxis) {
  ASSERT(ele);
  (void)snprintf(reinterpret_cast<char *>(ele->name), 2, "%-2s", name);
  ele->op = op;
  ele->type_ele = UI_ELE_OVAL;
  ele->layer = layer;
  ele->color = color;
  ele->width = width;
  ele->x_start = x_center;
  ele->y_start = y_center;
  ele->x_end = x_semiaxis;
  ele->y_end = y_semiaxis;
  return 0;
}

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
                   uint16_t y_center, uint16_t x_semiaxis,
                   uint16_t y_semiaxis) {
  ASSERT(ele);
  (void)snprintf(reinterpret_cast<char *>(ele->name), 2, "%-2s", name);
  ele->op = op;
  ele->type_ele = UI_ELE_ARC;
  ele->layer = layer;
  ele->color = color;
  ele->angle_start = angle_start;
  ele->angle_end = angle_end;
  ele->width = width;
  ele->x_start = x_center;
  ele->y_start = y_center;
  ele->x_end = x_semiaxis;
  ele->y_end = y_semiaxis;
  return 0;
}

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
                     uint16_t float_middle, uint16_t float_low) {
  ASSERT(ele);
  (void)snprintf(reinterpret_cast<char *>(ele->name), 2, "%-2s", name);
  ele->op = op;
  ele->type_ele = UI_ELE_FLOAT;
  ele->layer = layer;
  ele->color = color;
  ele->angle_start = font_size;
  ele->angle_end = digits;
  ele->width = width;
  ele->x_start = x_start;
  ele->y_start = y_start;
  ele->radius = float_high;
  ele->x_end = float_middle;
  ele->y_end = float_low;
  return 0;
}

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
                   uint16_t int32_t_low) {
  ASSERT(ele);
  (void)snprintf(reinterpret_cast<char *>(ele->name), 2, "%-2s", name);
  ele->op = op;
  ele->type_ele = UI_ELE_INT;
  ele->layer = layer;
  ele->color = color;
  ele->angle_start = font_size;
  ele->width = width;
  ele->x_start = x_start;
  ele->y_start = y_start;
  ele->radius = int32_t_high;
  ele->x_end = int32_t_middle;
  ele->y_end = int32_t_low;
  return 0;
}

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
                      uint16_t y_start, const char *str) {
  ASSERT(ele);
  (void)snprintf(reinterpret_cast<char *>(ele->graphic.name), 2, "%-2s", name);
  ele->graphic.op = op;
  ele->graphic.type_ele = UI_ELE_STR;
  ele->graphic.layer = layer;
  ele->graphic.color = color;
  ele->graphic.angle_start = font_size;
  ele->graphic.angle_end = length;
  ele->graphic.width = width;
  ele->graphic.x_start = x_start;
  ele->graphic.y_start = y_start;
  memset(ele->str, 0x00, sizeof(ele->str));
  (void)snprintf(reinterpret_cast<char *>(ele->str), strlen(str) + 1, "%-2s",
                 str);
  return 0;
}

/**
 * @brief UI_删除图层
 *
 * @param del 结构体
 * @param op 操作
 * @param layer 图层
 * @return int8_t
 */
int8_t ui_del_layer(ui_del_t *del, ui_del_op_t op, uint8_t layer) {
  ASSERT(del);
  del->op = op;
  del->layer = layer;
  return 0;
}
