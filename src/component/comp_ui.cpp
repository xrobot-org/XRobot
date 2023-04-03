/*
  UI相关命令
*/
#include "comp_ui.hpp"

using namespace Component;
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
int8_t Component::UI::Line::Draw(const char *name, GraphicOperation op,
                                 uint8_t layer, Color color, uint16_t width,
                                 uint16_t x_start, uint16_t y_start,
                                 uint16_t x_end, uint16_t y_end) {
  (void)snprintf(reinterpret_cast<char *>(ele_.name), 3, "%-3s", name);
  ele_.layer = layer;
  ele_.op = op;
  ele_.type_ele = UI_ELE_LINE;
  ele_.color = color;
  ele_.width = width;
  ele_.x_start = x_start;
  ele_.y_start = y_start;
  ele_.x_end = x_end;
  ele_.y_end = y_end;
  return 0;
}

/**
 * @brief UI_绘制矩形
 *
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
int8_t Component::UI::Rectangle::Draw(const char *name, GraphicOperation op,
                                      uint8_t layer, Color color,
                                      uint16_t width, uint16_t x_start,
                                      uint16_t y_start, uint16_t x_end,
                                      uint16_t y_end) {
  (void)snprintf(reinterpret_cast<char *>(ele_.name), 3, "%-3s", name);
  ele_.op = op;
  ele_.type_ele = UI_ELE_RECT;
  ele_.layer = layer;
  ele_.color = color;
  ele_.width = width;
  ele_.x_start = x_start;
  ele_.y_start = y_start;
  ele_.x_end = x_end;
  ele_.y_end = y_end;
  return 0;
}

/**
 * @brief UI_绘制正圆
 *
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
int8_t Component::UI::Cycle::Draw(const char *name, GraphicOperation op,
                                  uint8_t layer, Color color, uint16_t width,
                                  uint16_t x_center, uint16_t y_center,
                                  uint16_t radius) {
  (void)snprintf(reinterpret_cast<char *>(ele_.name), 3, "%-3s", name);
  ele_.op = op;
  ele_.layer = layer;
  ele_.type_ele = UI_ELE_CYCLE;
  ele_.color = color;
  ele_.width = width;
  ele_.x_start = x_center;
  ele_.y_start = y_center;
  ele_.radius = radius;
  return 0;
}

/**
 * @brief UI_绘制椭圆
 *
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
int8_t Component::UI::Oval::Draw(const char *name, GraphicOperation op,
                                 uint8_t layer, Color color, uint16_t width,
                                 uint16_t x_center, uint16_t y_center,
                                 uint16_t x_semiaxis, uint16_t y_semiaxis) {
  (void)snprintf(reinterpret_cast<char *>(ele_.name), 3, "%-3s", name);
  ele_.op = op;
  ele_.type_ele = UI_ELE_OVAL;
  ele_.layer = layer;
  ele_.color = color;
  ele_.width = width;
  ele_.x_start = x_center;
  ele_.y_start = y_center;
  ele_.x_end = x_semiaxis;
  ele_.y_end = y_semiaxis;
  return 0;
}

/**
 * @brief UI_绘制圆弧
 *
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
int8_t Component::UI::Arc::Draw(const char *name, GraphicOperation op,
                                uint8_t layer, Color color,
                                uint16_t angle_start, uint16_t angle_end,
                                uint16_t width, uint16_t x_center,
                                uint16_t y_center, uint16_t x_semiaxis,
                                uint16_t y_semiaxis) {
  (void)snprintf(reinterpret_cast<char *>(ele_.name), 3, "%-3s", name);
  ele_.op = op;
  ele_.type_ele = UI_ELE_ARC;
  ele_.layer = layer;
  ele_.color = color;
  ele_.angle_start = angle_start;
  ele_.angle_end = angle_end;
  ele_.width = width;
  ele_.x_start = x_center;
  ele_.y_start = y_center;
  ele_.x_end = x_semiaxis;
  ele_.y_end = y_semiaxis;
  return 0;
}

/**
 * @brief UI_绘制浮点数
 *
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
int8_t Component::UI::FloatNum::Draw(const char *name, GraphicOperation op,
                                     uint8_t layer, Color color,
                                     uint16_t font_size, uint16_t digits,
                                     uint16_t width, uint16_t x_start,
                                     uint16_t y_start, uint16_t float_high,
                                     uint16_t float_middle,
                                     uint16_t float_low) {
  (void)snprintf(reinterpret_cast<char *>(ele_.name), 3, "%-3s", name);
  ele_.op = op;
  ele_.type_ele = UI_ELE_FLOAT;
  ele_.layer = layer;
  ele_.color = color;
  ele_.angle_start = font_size;
  ele_.angle_end = digits;
  ele_.width = width;
  ele_.x_start = x_start;
  ele_.y_start = y_start;
  ele_.radius = float_high;
  ele_.x_end = float_middle;
  ele_.y_end = float_low;
  return 0;
}

/**
 * @brief UI_绘制整型数
 *
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
int8_t Component::UI::IntNum::Draw(
    const char *name, GraphicOperation op, uint8_t layer, Color color,
    uint16_t font_size, uint16_t width, uint16_t x_start, uint16_t y_start,
    uint16_t int32_t_high, uint16_t int32_t_middle, uint16_t int32_t_low) {
  (void)snprintf(reinterpret_cast<char *>(ele_.name), 3, "%-3s", name);
  ele_.op = op;
  ele_.type_ele = UI_ELE_INT;
  ele_.layer = layer;
  ele_.color = color;
  ele_.angle_start = font_size;
  ele_.width = width;
  ele_.x_start = x_start;
  ele_.y_start = y_start;
  ele_.radius = int32_t_high;
  ele_.x_end = int32_t_middle;
  ele_.y_end = int32_t_low;
  return 0;
}

/**
 * @brief UI_绘制字符
 *
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
int8_t Component::UI::String::Draw(const char *name, GraphicOperation op,
                                   uint8_t layer, Color color,
                                   uint16_t font_size, uint16_t length,
                                   uint16_t width, uint16_t x_start,
                                   uint16_t y_start, const char *str) {
  (void)snprintf(reinterpret_cast<char *>(str_.graphic.name), 3, "%-3s", name);
  str_.graphic.op = op;
  str_.graphic.type_ele = UI_ELE_STR;
  str_.graphic.layer = layer;
  str_.graphic.color = color;
  str_.graphic.angle_start = font_size;
  str_.graphic.angle_end = length;
  str_.graphic.width = width;
  str_.graphic.x_start = x_start;
  str_.graphic.y_start = y_start;
  memset(str_.str, 0x00, sizeof(str_.str));
  (void)snprintf(reinterpret_cast<char *>(str_.str), strlen(str) + 1, "%-3s",
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
int8_t Component::UI::Delete::Draw(DelOperation op, uint8_t layer) {
  del_.op = op;
  del_.layer = layer;
  return 0;
}
