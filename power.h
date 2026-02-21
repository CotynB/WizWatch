#pragma once

#include "pin_config.h"
#include <XPowersLib.h>

extern XPowersPMU PMU;

#define INACTIVITY_TIMEOUT_MS 20000  // 20 seconds

void power_init();
void power_check_button();
void power_sleep();
void power_wake();
bool power_is_sleeping();
void power_optimize_idle();    // Call in loop to reduce CPU when idle
void power_reset_inactivity(); // Call on user activity (touch, notification)
void power_check_inactivity(); // Call in loop to auto-sleep
bool power_use_light_sleep();  // True if should use esp_light_sleep_start()
