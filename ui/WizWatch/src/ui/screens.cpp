#include <string.h>

#include "screens.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;

static void event_handler_cb_main_settings_btt(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, -1, 2, e);
    }
}

static void event_handler_cb_settings_brightnessslider(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_slider_get_value(ta);
            assignIntegerProperty(flowState, 2, 3, value, "Failed to assign Value in Slider widget");
        }
    }
}

static void event_handler_cb_settings_settings_btt_1(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, -1, 3, e);
    }
}

void create_screen_main() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 410, 502);
    lv_obj_set_style_transform_scale_x(obj, 256, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // FOND
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fond = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, "S:fond.bin");
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // BatteryFull
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.battery_full = obj;
            lv_obj_set_pos(obj, 70, 33);
            lv_obj_set_size(obj, 36, 54);
            lv_image_set_src(obj, "S:full.bin");
            lv_image_set_scale(obj, 200);
        }
        {
            // BatteryHalfFull
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.battery_half_full = obj;
            lv_obj_set_pos(obj, 71, 34);
            lv_obj_set_size(obj, 36, 54);
            lv_image_set_src(obj, "S:half_full.bin");
            lv_image_set_scale(obj, 200);
        }
        {
            // BatteryEmpty
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.battery_empty = obj;
            lv_obj_set_pos(obj, 71, 34);
            lv_obj_set_size(obj, 36, 54);
            lv_image_set_src(obj, "S:empty.bin");
            lv_image_set_scale(obj, 200);
        }
        {
            // TimeLBL
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.time_lbl = obj;
            lv_obj_set_pos(obj, 39, 144);
            lv_obj_set_size(obj, 333, 43);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffb4e898), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // SettingsBTT
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.settings_btt = obj;
            lv_obj_set_pos(obj, 58, 392);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 60);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "S:setttings_icon.bin", NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_main_settings_btt, LV_EVENT_ALL, flowState);
        }
    }
    
    tick_screen_main();
}

void delete_screen_main() {
    lv_obj_delete(objects.main);
    objects.main = 0;
    objects.fond = 0;
    objects.battery_full = 0;
    objects.battery_half_full = 0;
    objects.battery_empty = 0;
    objects.time_lbl = 0;
    objects.settings_btt = 0;
    deletePageFlowState(0);
}

void tick_screen_main() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
    {
        bool new_val = evalBooleanProperty(flowState, 2, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.battery_full, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.battery_full;
            if (new_val) lv_obj_add_flag(objects.battery_full, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.battery_full, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 3, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.battery_half_full, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.battery_half_full;
            if (new_val) lv_obj_add_flag(objects.battery_half_full, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.battery_half_full, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        bool new_val = evalBooleanProperty(flowState, 4, 3, "Failed to evaluate Hidden flag");
        bool cur_val = lv_obj_has_flag(objects.battery_empty, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.battery_empty;
            if (new_val) lv_obj_add_flag(objects.battery_empty, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.battery_empty, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 5, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.time_lbl);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.time_lbl;
            lv_label_set_text(objects.time_lbl, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_settings() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 410, 502);
    {
        lv_obj_t *parent_obj = obj;
        {
            // FOND_1
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fond_1 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, "S:fond.bin");
            {
                lv_obj_t *parent_obj = obj;
                {
                    // Brightnessslider
                    lv_obj_t *obj = lv_slider_create(parent_obj);
                    objects.brightnessslider = obj;
                    lv_obj_set_pos(obj, 130, 114);
                    lv_obj_set_size(obj, 150, 30);
                    lv_obj_add_event_cb(obj, event_handler_cb_settings_brightnessslider, LV_EVENT_ALL, flowState);
                    lv_obj_set_style_outline_color(obj, lv_color_hex(0xff000000), LV_PART_KNOB | LV_STATE_SCROLLED);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff86b66d), LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb4e898), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff76a060), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
            }
        }
        {
            // BrightnessLBL
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.brightness_lbl = obj;
            lv_obj_set_pos(obj, 39, 62);
            lv_obj_set_size(obj, 333, 43);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffb4e898), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Luminosite\n");
        }
        {
            // BrightnessLBL_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.brightness_lbl_1 = obj;
            lv_obj_set_pos(obj, 39, 156);
            lv_obj_set_size(obj, 333, 43);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffb4e898), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            // SettingsBTT_1
            lv_obj_t *obj = lv_imagebutton_create(parent_obj);
            objects.settings_btt_1 = obj;
            lv_obj_set_pos(obj, 58, 392);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 60);
            lv_imagebutton_set_src(obj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, "S:home_icon.bin", NULL);
            lv_obj_add_event_cb(obj, event_handler_cb_settings_settings_btt_1, LV_EVENT_ALL, flowState);
        }
    }
    
    tick_screen_settings();
}

void delete_screen_settings() {
    lv_obj_delete(objects.settings);
    objects.settings = 0;
    objects.fond_1 = 0;
    objects.brightnessslider = 0;
    objects.brightness_lbl = 0;
    objects.brightness_lbl_1 = 0;
    objects.settings_btt_1 = 0;
    deletePageFlowState(1);
}

void tick_screen_settings() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    {
        int32_t new_val = evalIntegerProperty(flowState, 2, 3, "Failed to evaluate Value in Slider widget");
        int32_t cur_val = lv_slider_get_value(objects.brightnessslider);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.brightnessslider;
            lv_slider_set_value(objects.brightnessslider, new_val, LV_ANIM_ON);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = evalTextProperty(flowState, 4, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.brightness_lbl_1);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.brightness_lbl_1;
            lv_label_set_text(objects.brightness_lbl_1, new_val);
            tick_value_change_obj = NULL;
        }
    }
}


static const char *screen_names[] = { "Main", "Settings" };
static const char *object_names[] = { "main", "settings", "settings_btt", "settings_btt_1", "fond", "battery_full", "battery_half_full", "battery_empty", "time_lbl", "fond_1", "brightnessslider", "brightness_lbl", "brightness_lbl_1" };


typedef void (*create_screen_func_t)();
create_screen_func_t create_screen_funcs[] = {
    create_screen_main,
    create_screen_settings,
};
void create_screen(int screen_index) {
    create_screen_funcs[screen_index]();
}
void create_screen_by_id(enum ScreensEnum screenId) {
    create_screen_funcs[screenId - 1]();
}

typedef void (*delete_screen_func_t)();
delete_screen_func_t delete_screen_funcs[] = {
    delete_screen_main,
    delete_screen_settings,
};
void delete_screen(int screen_index) {
    delete_screen_funcs[screen_index]();
}
void delete_screen_by_id(enum ScreensEnum screenId) {
    delete_screen_funcs[screenId - 1]();
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_settings,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    eez_flow_init_screen_names(screen_names, sizeof(screen_names) / sizeof(const char *));
    eez_flow_init_object_names(object_names, sizeof(object_names) / sizeof(const char *));
    
    eez_flow_set_create_screen_func(create_screen);
    eez_flow_set_delete_screen_func(delete_screen);
    
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
    create_screen_settings();
}
