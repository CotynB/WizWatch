#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;

void create_screen_main() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 410, 502);
    {
        lv_obj_t *parent_obj = obj;
        {
            // FOND
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.fond = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_fond);
        }
        {
            // BatteryFull
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.battery_full = obj;
            lv_obj_set_pos(obj, 70, 33);
            lv_obj_set_size(obj, 36, 54);
            lv_image_set_src(obj, &img_full);
            lv_image_set_scale(obj, 200);
        }
        {
            // BatteryHalfFull
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.battery_half_full = obj;
            lv_obj_set_pos(obj, 71, 34);
            lv_obj_set_size(obj, 36, 54);
            lv_image_set_src(obj, &img_half_full);
            lv_image_set_scale(obj, 200);
        }
        {
            // BatteryEmpty
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.battery_empty = obj;
            lv_obj_set_pos(obj, 71, 34);
            lv_obj_set_size(obj, 36, 54);
            lv_image_set_src(obj, &img_empty);
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


static const char *screen_names[] = { "Main" };
static const char *object_names[] = { "main", "fond", "battery_full", "battery_half_full", "battery_empty", "time_lbl" };


typedef void (*create_screen_func_t)();
create_screen_func_t create_screen_funcs[] = {
    create_screen_main,
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
}
