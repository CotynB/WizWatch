#include <string.h>
#include "vars.h"

// Native variable for RTC time display
char rtc_time[100] = { 0 };

extern "C" {

const char *get_var_rtc_time() {
    return rtc_time;
}

void set_var_rtc_time(const char *value) {
    strncpy(rtc_time, value, sizeof(rtc_time) / sizeof(char));
    rtc_time[sizeof(rtc_time) / sizeof(char) - 1] = 0;
}

}
