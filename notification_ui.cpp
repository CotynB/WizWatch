#include "notification_ui.h"
#include <lvgl.h>
#include "ui/WizWatch/src/ui/fonts.h"
#include "power.h"

// Dimensions
#define NOTIF_WIDTH      380
#define NOTIF_HEIGHT     105
#define NOTIF_MARGIN_X   15   // (410 - 380) / 2
#define NOTIF_ANIM_MS    300
#define NOTIF_GAP        8
#define MAX_CARDS        3

// Colors (matching green theme)
#define COLOR_BG         lv_color_hex(0x1a1a2e)
#define COLOR_ACCENT     lv_color_hex(0x86b66d)
#define COLOR_SRC        lv_color_hex(0xb4e898)
#define COLOR_TITLE      lv_color_hex(0xffffff)
#define COLOR_BODY       lv_color_hex(0xaaaaaa)

// Container for all notification cards
static lv_obj_t *container = nullptr;
static bool sleepWakeBg = false;

// Forward declarations
static lv_obj_t* create_card(const char* src, const char* title, const char* body);
static void card_tap_cb(lv_event_t *e);
static void card_dismiss_anim_cb(lv_anim_t *a);
static void remove_oldest_card();

void notification_ui_init() {
    // Scrollable container on top layer
    container = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(container, 0, 0);
    lv_obj_set_size(container, 410, 502);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(container, 55, LV_PART_MAIN);
    lv_obj_set_style_pad_row(container, NOTIF_GAP, LV_PART_MAIN);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);

    // Let touch events pass through to the screen below
    lv_obj_clear_flag(container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_EVENT_BUBBLE);
}

void notification_ui_set_sleep_bg(bool on) {
    if (!container) return;
    sleepWakeBg = on;
    if (on) {
        lv_obj_set_style_bg_color(container, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(container, LV_OPA_COVER, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    }
}

void notification_ui_show(const char* src, const char* title, const char* body) {
    if (!container) return;

    // Enforce max card limit
    while (lv_obj_get_child_count(container) >= MAX_CARDS) {
        remove_oldest_card();
    }

    // Create and animate new card
    lv_obj_t *card = create_card(src, title, body);

    // Slide-down animation
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, card);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_values(&anim, -(NOTIF_HEIGHT + 10), lv_obj_get_y(card));
    lv_anim_set_duration(&anim, NOTIF_ANIM_MS);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_start(&anim);
}

void notification_ui_dismiss() {
    if (!container || lv_obj_get_child_count(container) == 0) return;

    // Dismiss the newest (last) card
    lv_obj_t *card = lv_obj_get_child(container, lv_obj_get_child_count(container) - 1);
    if (card) {
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, card);
        lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
        lv_anim_set_values(&anim, lv_obj_get_x(card), -NOTIF_WIDTH);
        lv_anim_set_duration(&anim, NOTIF_ANIM_MS);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
        lv_anim_set_completed_cb(&anim, card_dismiss_anim_cb);
        lv_anim_start(&anim);
    }
}

static lv_obj_t* create_card(const char* src, const char* title, const char* body) {
    lv_obj_t *card = lv_obj_create(container);
    lv_obj_set_size(card, NOTIF_WIDTH, NOTIF_HEIGHT);
    lv_obj_set_style_bg_color(card, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 16, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
    lv_obj_set_style_pad_left(card, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_right(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_top(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(card, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 4, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Tap to dismiss this card
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, card_tap_cb, LV_EVENT_CLICKED, nullptr);

    // App name
    lv_obj_t *srcLbl = lv_label_create(card);
    lv_obj_set_width(srcLbl, NOTIF_WIDTH - 32);
    lv_obj_set_style_text_font(srcLbl, &ui_font_dot_gothic16_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(srcLbl, COLOR_SRC, LV_PART_MAIN);
    lv_label_set_text(srcLbl, src ? src : "");

    // Title
    lv_obj_t *titleLbl = lv_label_create(card);
    lv_obj_set_width(titleLbl, NOTIF_WIDTH - 32);
    lv_obj_set_style_text_font(titleLbl, &ui_font_dot_gothic16_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(titleLbl, COLOR_TITLE, LV_PART_MAIN);
    lv_label_set_long_mode(titleLbl, LV_LABEL_LONG_DOT);
    lv_label_set_text(titleLbl, title ? title : "");

    // Body (2 lines max)
    lv_obj_t *bodyLbl = lv_label_create(card);
    lv_obj_set_width(bodyLbl, NOTIF_WIDTH - 32);
    lv_obj_set_height(bodyLbl, 20);
    lv_obj_set_style_text_font(bodyLbl, &ui_font_dot_gothic16_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(bodyLbl, COLOR_BODY, LV_PART_MAIN);
    lv_label_set_long_mode(bodyLbl, LV_LABEL_LONG_DOT);
    lv_label_set_text(bodyLbl, body ? body : "");

    return card;
}

static void card_tap_cb(lv_event_t *e) {
    lv_obj_t *card = (lv_obj_t *)lv_event_get_target(e);

    // Slide left to dismiss
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, card);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_values(&anim, lv_obj_get_x(card), -NOTIF_WIDTH);
    lv_anim_set_duration(&anim, NOTIF_ANIM_MS);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&anim, card_dismiss_anim_cb);
    lv_anim_start(&anim);
}

static void card_dismiss_anim_cb(lv_anim_t *a) {
    lv_obj_t *card = (lv_obj_t *)a->var;
    lv_obj_delete(card);

    // If woke just for notifications and all dismissed, go back to sleep
    if (sleepWakeBg && lv_obj_get_child_count(container) == 0) {
        notification_ui_set_sleep_bg(false);
        power_sleep();
    }
}

static void remove_oldest_card() {
    if (lv_obj_get_child_count(container) == 0) return;
    lv_obj_t *oldest = lv_obj_get_child(container, 0);
    lv_obj_delete(oldest);
}
