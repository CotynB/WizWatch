#pragma once

void brightness_init();
void brightness_update();          // Only updates display if value changed
void brightness_force_update();    // Force update (call after wake)
void brightness_set(int level);    // Set brightness 0-100
