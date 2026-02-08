#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"

extern std::unique_ptr<Arduino_IIC> FT3168;

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);
void touch_init();
bool touch_has_activity();  // Check if touch occurred (clears flag)
