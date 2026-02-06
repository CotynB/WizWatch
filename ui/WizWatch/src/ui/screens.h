#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *settings;
    lv_obj_t *settings_btt;
    lv_obj_t *settings_btt_1;
    lv_obj_t *fond;
    lv_obj_t *battery_full;
    lv_obj_t *battery_half_full;
    lv_obj_t *battery_empty;
    lv_obj_t *time_lbl;
    lv_obj_t *fond_1;
    lv_obj_t *brightnessslider;
    lv_obj_t *brightness_lbl;
    lv_obj_t *brightness_lbl_1;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_SETTINGS = 2,
};

void create_screen_main();
void delete_screen_main();
void tick_screen_main();

void create_screen_settings();
void delete_screen_settings();
void tick_screen_settings();

void create_screen_by_id(enum ScreensEnum screenId);
void delete_screen_by_id(enum ScreensEnum screenId);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/