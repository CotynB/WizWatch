#pragma once
#include <Arduino.h>
#include <lvgl.h>

bool sd_card_init();
void sd_card_sleep();  // Unmount SD card to save power
bool sd_card_wake();   // Remount SD card after sleep
