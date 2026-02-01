#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "SensorPCF85063.hpp"

extern SensorPCF85063 rtc;

void rtc_init();
void rtc_update_label(lv_obj_t *label);
