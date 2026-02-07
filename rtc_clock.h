#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "SensorPCF85063.hpp"

extern SensorPCF85063 rtc;

void rtc_init();
void rtc_tick();
void rtc_update_display();  // Force immediate display update
void rtc_set_from_epoch(long epoch);  // Set RTC from unix timestamp
