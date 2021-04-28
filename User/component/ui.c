/*
  UI相关命令
*/
#include "component\ui.h"

#include <stdio.h>

typedef enum {
  UI_ELE_LINE = 0,
  UI_ELE_RECT,
  UI_ELE_CYCLE,
  UI_ELE_OVAL,
  UI_ELE_ARC,
  UI_ELE_FLOAT,
  UI_ELE_INT,
  UI_ELE_STR,
} UI_EleType_t;

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
int8_t UI_DrawLine(UI_Ele_t *ele, const char *name, UI_GraphicOp_t op,
                   uint8_t layer, UI_Color_t color, uint16_t width,
                   uint16_t x_start, uint16_t y_start, uint16_t x_end,
                   uint16_t y_end) {
  ASSERT(ele);
  snprintf((char *)ele->name, 2, "%s", name);
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
int8_t UI_DrawRectangle(UI_Ele_t *ele, const char *name, UI_GraphicOp_t op,
                        uint8_t layer, UI_Color_t color, uint16_t width,
                        uint16_t x_start, uint16_t y_start, uint16_t x_end,
                        uint16_t y_end) {
  ASSERT(ele);
  snprintf((char *)ele->name, 2, "%s", name);
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
int8_t UI_DrawCycle(UI_Ele_t *ele, const char *name, UI_GraphicOp_t op,
                    uint8_t layer, UI_Color_t color, uint16_t width,
                    uint16_t x_center, uint16_t y_center, uint16_t radius) {
  ASSERT(ele);
  snprintf((char *)ele->name, 2, "%s", name);
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
int8_t UI_DrawOval(UI_Ele_t *ele, const char *name, UI_GraphicOp_t op,
                   uint8_t layer, UI_Color_t color, uint16_t width,
                   uint16_t x_center, uint16_t y_center, uint16_t x_semiaxis,
                   uint16_t y_semiaxis) {
  ASSERT(ele);
  snprintf((char *)ele->name, 2, "%s", name);
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
int8_t UI_DrawArc(UI_Ele_t *ele, const char *name, UI_GraphicOp_t op,
                  uint8_t layer, UI_Color_t color, uint16_t angle_start,
                  uint16_t angle_end, uint16_t width, uint16_t x_center,
                  uint16_t y_center, uint16_t x_semiaxis, uint16_t y_semiaxis) {
  ASSERT(ele);
  snprintf((char *)ele->name, 2, "%s", name);
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
int8_t UI_DrawFloating(UI_Ele_t *ele, const char *name, UI_GraphicOp_t op,
                       uint8_t layer, UI_Color_t color, uint16_t font_size,
                       uint16_t digits, uint16_t width, uint16_t x_start,
                       uint16_t y_start, uint16_t float_high,
                       uint16_t float_middle, uint16_t float_low) {
  ASSERT(ele);
  snprintf((char *)ele->name, 2, "%s", name);
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
int8_t UI_DrawInteger(UI_Ele_t *ele, const char *name, UI_GraphicOp_t op,
                      uint8_t layer, UI_Color_t color, uint16_t font_size,
                      uint16_t width, uint16_t x_start, uint16_t y_start,
                      uint16_t int32_t_high, uint16_t int32_t_middle,
                      uint16_t int32_t_low) {
  ASSERT(ele);
  snprintf((char *)ele->name, 2, "%s", name);
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
int8_t UI_DrawString(UI_String_t *ele, const char *name, UI_GraphicOp_t op,
                     uint8_t layer, UI_Color_t color, uint16_t font_size,
                     uint16_t length, uint16_t width, uint16_t x_start,
                     uint16_t y_start, const char *str) {
  ASSERT(ele);
  snprintf((char *)ele->graphic.name, 2, "%s", name);
  ele->graphic.op = op;
  ele->graphic.type_ele = UI_ELE_STR;
  ele->graphic.layer = layer;
  ele->graphic.color = color;
  ele->graphic.angle_start = font_size;
  ele->graphic.angle_end = length;
  ele->graphic.width = width;
  ele->graphic.x_start = x_start;
  ele->graphic.y_start = y_start;
  snprintf((char *)ele->str, 29, "%s", str);
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
int8_t UI_DelLayer(UI_Del_t *del, UI_DelOp_t op, uint8_t layer) {
  ASSERT(del);
  del->op = op;
  del->layer = layer;
  return 0;
}

int8_t UI_StashGraphic(UI_t *ui, const UI_Ele_t *ele) {
  if (ui->stack.size.graphic < UI_MAX_GRAPHIC_NUM) {
    memcpy(ui->stack.data.graphic + ui->stack.size.graphic, ele,
           sizeof(UI_Ele_t));
    ui->stack.size.graphic++;
    return 0;
  } else {
    return -1;
  }
}

int8_t UI_PopGraphic(UI_Ele_t *ele, UI_t *ui) {
  if (ui->stack.size.graphic) {
    memcpy(ele, ui->stack.data.graphic + ui->stack.size.graphic,
           sizeof(UI_Ele_t));
    ui->stack.size.graphic--;
    return 0;
  } else {
    return -1;
  }
}

int8_t UI_StashString(UI_t *ui, const UI_String_t *string) {
  if (ui->stack.size.string < UI_MAX_STRING_NUM) {
    memcpy(ui->stack.data.string + ui->stack.size.string, string,
           sizeof(UI_String_t));
    ui->stack.size.string++;
    return 0;
  } else {
    return -1;
  }
}

int8_t UI_PopString(UI_String_t *string, UI_t *ui) {
  if (ui->stack.size.string) {
    memcpy(string, ui->stack.data.string + ui->stack.size.string,
           sizeof(UI_String_t));
    ui->stack.size.string--;
    return 0;
  } else {
    return -1;
  }
}

int8_t UI_StashDel(UI_t *ui, const UI_Del_t *del) {
  if (ui->stack.size.del < UI_MAX_DEL_NUM) {
    memcpy(ui->stack.data.del + ui->stack.size.del, del, sizeof(UI_Del_t));
    ui->stack.size.del++;
    return 0;
  } else {
    return -1;
  }
}

int8_t UI_PopDel(UI_Del_t *del, UI_t *ui) {
  if (ui->stack.size.del < UI_MAX_DEL_NUM) {
    memcpy(del, ui->stack.data.del + ui->stack.size.del, sizeof(UI_Del_t));
    ui->stack.size.del--;
    return 0;
  } else {
    return -1;
  }
}

void UI_EmptyStash(UI_t *ui) {
  ui->stack.size.graphic = 0;
  ui->stack.size.string = 0;
  ui->stack.size.del = 0;
}
