#pragma once

// Native variables for EEZ Flow
// Include this in your main sketch after rtc_clock.h

#include "rtc_clock.h"
#include "ui/WizWatch/src/ui/ui.h"

// Getter function for time string
extern "C" void* get_rtc_time_string() {
    return (void*)rtc_time_string;
}

// Note: You need to register this in EEZ Studio:
// 1. Open your .eez-project
// 2. Add a Native Variable called "rtc_time"
// 3. Type: String
// 4. Get function: get_rtc_time_string
// 5. Then bind your time label to this variable
