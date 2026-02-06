#pragma once

void power_init();
void power_check_button();
void power_sleep();
void power_wake();
bool power_is_sleeping();
void power_optimize_idle();  // Call in loop to reduce power when idle
