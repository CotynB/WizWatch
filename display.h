#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"

extern Arduino_DataBus *bus;
extern Arduino_GFX *gfx;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void rounder_event_cb(lv_event_t *e);
void display_init();
