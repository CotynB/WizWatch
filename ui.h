#pragma once
#include <lvgl.h>

typedef struct {
  lv_obj_t *main;
  lv_obj_t *fond;
  lv_obj_t *battery_full;
  lv_obj_t *battery_half_full;
  lv_obj_t *battery_empty;
  lv_obj_t *time_lbl;
} ui_objects_t;

extern ui_objects_t ui_objects;

void ui_create();
void ui_tick();
void ui_set_battery_state(int state);
