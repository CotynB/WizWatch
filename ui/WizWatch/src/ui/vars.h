#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_BATTERY_STATE = 0,
    FLOW_GLOBAL_VARIABLE_TIME_TEXT = 1,
    FLOW_GLOBAL_VARIABLE_BRIGHTNESS = 2
};

// Native global variables

extern const char *get_var_rtc_time();
extern void set_var_rtc_time(const char *value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/