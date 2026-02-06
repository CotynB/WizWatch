#pragma once

// Battery state values for EEZ Flow
enum BatteryState {
    BATTERY_EMPTY = 0,
    BATTERY_HALF = 1,
    BATTERY_FULL = 2
};

void battery_init();
void battery_update();
int get_battery_level();  // Returns 0-100
