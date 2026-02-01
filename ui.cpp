#include "ui.h"

lv_obj_t *label;
lv_obj_t *btn;
lv_obj_t *btnLabel;

static void btn_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_label_set_text(label, "Button Pressed!");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_40, LV_PART_MAIN);
  }
}

void ui_create() {
  label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Initializing...");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  btn = lv_btn_create(lv_scr_act());
  lv_obj_set_size(btn, 200, 60);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 80);
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

  btnLabel = lv_label_create(btn);
  lv_label_set_text(btnLabel, "Press Me");
  lv_obj_center(btnLabel);
}
