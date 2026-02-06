#include "ui.h"
#include "rtc_clock.h"
#include "HWCDC.h"

extern HWCDC USBSerial;

ui_objects_t ui_objects;

// Battery state: 0 = empty, 1 = half, 2 = full
static int current_battery_state = 2;

static void update_battery_icons() {
  if (current_battery_state == 2) {
    lv_obj_clear_flag(ui_objects.battery_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_objects.battery_half_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_objects.battery_empty, LV_OBJ_FLAG_HIDDEN);
  } else if (current_battery_state == 1) {
    lv_obj_add_flag(ui_objects.battery_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_objects.battery_half_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_objects.battery_empty, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(ui_objects.battery_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_objects.battery_half_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_objects.battery_empty, LV_OBJ_FLAG_HIDDEN);
  }
}

void ui_set_battery_state(int state) {
  if (state < 0) state = 0;
  if (state > 2) state = 2;
  current_battery_state = state;
  update_battery_icons();
}

void ui_create() {
  lv_obj_t *scr = lv_obj_create(0);
  ui_objects.main = scr;
  lv_obj_set_pos(scr, 0, 0);
  lv_obj_set_size(scr, 410, 502);

  // Background image
  {
    lv_obj_t *obj = lv_image_create(scr);
    ui_objects.fond = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_image_set_src(obj, "S:fond.bin");
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  // Battery full icon
  {
    lv_obj_t *obj = lv_image_create(scr);
    ui_objects.battery_full = obj;
    lv_obj_set_pos(obj, 70, 33);
    lv_obj_set_size(obj, 36, 54);
    lv_image_set_src(obj, "S:full.bin");
    USBSerial.println("Battery full image loaded");
    lv_image_set_scale(obj, 200);
  }

  // Battery half full icon
  {
    lv_obj_t *obj = lv_image_create(scr);
    ui_objects.battery_half_full = obj;
    lv_obj_set_pos(obj, 71, 34);
    lv_obj_set_size(obj, 36, 54);
    lv_image_set_src(obj, "S:half_full.bin");
    lv_image_set_scale(obj, 200);
  }

  // Battery empty icon
  {
    lv_obj_t *obj = lv_image_create(scr);
    ui_objects.battery_empty = obj;
    lv_obj_set_pos(obj, 71, 34);
    lv_obj_set_size(obj, 36, 54);
    lv_image_set_src(obj, "S:empty.bin");
    lv_image_set_scale(obj, 200);
  }

  // Time label
  {
    lv_obj_t *obj = lv_label_create(scr);
    ui_objects.time_lbl = obj;
    lv_obj_set_size(obj, 333, 43);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 134);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xffb4e898), LV_PART_MAIN | LV_STATE_DEFAULT), LV_PART_MAIN | LV_STATE_DEFAULT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, "");
    lv_obj_set_pos(obj, 39, 144);
  }

  update_battery_icons();
  lv_scr_load(scr);
}

void ui_tick() {
  rtc_update_label(ui_objects.time_lbl);
}
