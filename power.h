#pragma once

#define INACTIVITY_TIMEOUT_MS 30000  // 30 seconds

void power_init();
void power_check_button();
void power_sleep();
void power_wake();
bool power_is_sleeping();
void power_optimize_idle();  // Call in loop to reduce power when idle
void power_reset_inactivity();  // Call on user activity (touch, notification)
void power_check_inactivity();  // Call in loop to auto-sleep
