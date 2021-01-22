/*
  UI相关命令
*/

#include "ui.h"

void UI_DrawLine(UI_Ele_t *grapic_line, const char *name, uint8_t type_op,
                 uint8_t layer, uint8_t color, uint16_t width, uint16_t x_start,
                 uint16_t y_start, uint16_t x_end, uint16_t y_end) {
  snprintf(grapic_line->name, 2, "%s", name);
  grapic_line->layer = layer;
  grapic_line->type_op = type_op;
  grapic_line->type_ele = 0;
  grapic_line->color = color;
  grapic_line->width = width;
  grapic_line->x_start = x_start;
  grapic_line->y_start = y_start;
  grapic_line->x_end = x_end;
  grapic_line->y_end = y_end;
}

void UI_DrawRectangle(UI_Ele_t *grapic_rectangle, const char *name,
                      uint8_t type_op, uint8_t layer, uint8_t color,
                      uint16_t width, uint16_t x_start, uint16_t y_start,
                      uint16_t x_end, uint16_t y_end) {
  snprintf(grapic_rectangle->name, 2, "%s", name);
  grapic_rectangle->type_op = type_op;
  grapic_rectangle->type_ele = 1;
  grapic_rectangle->layer = layer;
  grapic_rectangle->color = color;
  grapic_rectangle->width = width;
  grapic_rectangle->x_start = x_start;
  grapic_rectangle->y_start = y_start;
  grapic_rectangle->x_end = x_end;
  grapic_rectangle->y_end = y_end;
}

void UI_DrawCycle(UI_Ele_t *grapic_cycle, const char *name, uint8_t type_op,
                  uint8_t layer, uint8_t color, uint16_t width,
                  uint16_t x_center, uint16_t y_center, uint16_t radius) {
  snprintf(grapic_cycle->name, 2, "%s", name);
  grapic_cycle->type_op = type_op;
  grapic_cycle->layer = layer;
  grapic_cycle->type_ele = 2;
  grapic_cycle->color = color;
  grapic_cycle->width = width;
  grapic_cycle->x_start = x_center;
  grapic_cycle->y_start = y_center;
  grapic_cycle->radius = radius;
}

void UI_DrawOval(UI_Ele_t *grapic_oval, const char *name, uint8_t type_op,
                 uint8_t layer, uint8_t color, uint16_t width,
                 uint16_t x_center, uint16_t y_center, uint16_t x_semiaxis,
                 uint16_t y_semiaxis) {
  snprintf(grapic_oval->name, 2, "%s", name);
  grapic_oval->type_op = type_op;
  grapic_oval->type_ele = 3;
  grapic_oval->layer = layer;
  grapic_oval->color = color;
  grapic_oval->width = width;
  grapic_oval->x_start = x_center;
  grapic_oval->y_start = y_center;
  grapic_oval->x_end = x_semiaxis;
  grapic_oval->y_end = y_semiaxis;
}

void UI_DrawArc(UI_Ele_t *grapic_arc, const char *name, uint8_t type_op,
                uint8_t layer, uint8_t color, uint16_t angle_start,
                uint16_t angle_end, uint16_t width, uint16_t x_center,
                uint16_t y_center, uint16_t x_semiaxis, uint16_t y_semiaxis) {
  snprintf(grapic_arc->name, 2, "%s", name);
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
}

void UI_DrawFloating(UI_Ele_t *grapic_floating, const char *name,
                     uint8_t type_op, uint8_t layer, uint8_t color,
                     uint16_t font_size, uint16_t digits, uint16_t width,
                     uint16_t x_start, uint16_t y_start, uint16_t float_high,
                     uint16_t float_middle, uint16_t float_low) {
  snprintf(grapic_floating->name, 2, "%s", name);
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
}

void UI_DrawInteger(UI_Ele_t *grapic_integer, const char *name, uint8_t type_op,
                    uint8_t layer, uint8_t color, uint16_t font_size,
                    uint16_t width, uint16_t x_start, uint16_t y_start,
                    uint16_t int32_t_high, uint16_t int32_t_middle,
                    uint16_t int32_t_low) {
  snprintf(grapic_integer->name, 2, "%s", name);
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
}

void UI_DrawCharacter(UI_Drawcharacter_t *grapic_character, const char *name,
                      uint8_t type_op, uint8_t layer, uint8_t color,
                      uint16_t font_size, uint16_t length, uint16_t width,
                      uint16_t x_start, uint16_t y_start,
                      const char *character) {
  snprintf(grapic_character->grapic_t.name, 2, "%s", name);
  grapic_character->grapic_t.type_op = type_op;
  grapic_character->grapic_t.type_ele = 7;
  grapic_character->grapic_t.layer = layer;
  grapic_character->grapic_t.color = color;
  grapic_character->grapic_t.angle_start = font_size;
  grapic_character->grapic_t.angle_end = length;
  grapic_character->grapic_t.width = width;
  grapic_character->grapic_t.x_start = x_start;
  grapic_character->grapic_t.y_start = y_start;
  snprintf(grapic_character->character, 29, "%s", character);
}