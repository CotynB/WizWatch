#include <Wire.h>
#include <Arduino.h>
#include "pin_config.h"
#include <lvgl.h>
#include "lv_conf.h"
#include "HWCDC.h"
#include <WiFi.h>

#include "display.h"
#include "touch.h"
#include "rtc_clock.h"
#include "sd_card.h"
#include "battery.h"
#include "brightness.h"
#include "power.h"

// EEZ Studio generated UI
#include "ui/WizWatch/src/ui/ui.h"
#include "ui/WizWatch/src/ui/vars.h"

HWCDC USBSerial;

uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_display_t *disp;
lv_color_t *disp_draw_buf;

uint32_t millis_cb(void) {
  return millis();
}

#if LV_USE_LOG != 0
void my_print(lv_log_level_t level, const char *buf) {
  LV_UNUSED(level);
  USBSerial.println(buf);
  USBSerial.flush();
}
#endif

void setup() {
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif

  USBSerial.begin(115200);
  USBSerial.println("WizWatch starting...");

  // Disable WiFi and Bluetooth for maximum power saving
  WiFi.mode(WIFI_OFF);
  btStop();
  USBSerial.println("WiFi/BT disabled for power saving");

  display_init();
  touch_init();
  rtc_init();
  power_init();  // Initialize power button

  lv_init();
  lv_tick_set_cb(millis_cb);

  sd_card_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print);
#endif

  screenWidth = gfx->width();
  screenHeight = gfx->height();

#ifdef DIRECT_RENDER_MODE
  bufSize = screenWidth * screenHeight;
#else
  bufSize = screenWidth * 80;  // Increased from 50 to 80 for smoother animations
#endif

#ifdef ESP32
#if defined(DIRECT_RENDER_MODE) && (defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL))
  disp_draw_buf = (lv_color_t *)gfx->getFramebuffer();
#else
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf) {
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  }
#endif
#else
  disp_draw_buf = (lv_color_t *)malloc(bufSize * 2);
#endif
  if (!disp_draw_buf) {
    USBSerial.println("LVGL disp_draw_buf allocate failed!");
  } else {
    disp = lv_display_create(screenWidth, screenHeight);
    lv_display_set_flush_cb(disp, my_disp_flush);
#ifdef DIRECT_RENDER_MODE
    lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_DIRECT);
#else
    lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    lv_display_add_event_cb(disp, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);

    ui_init();

    // Initialize battery and brightness after UI/Flow system is ready
    battery_init();
    brightness_init();

    // Set initial time to avoid flicker from default value
    rtc_update_display();
  }

  USBSerial.println("Ready");
}

void loop() {
  static uint32_t lastBatteryUpdate = 0;
  uint32_t now = millis();

  // Always check power button (wake source)
  power_check_button();

  // Skip most processing if sleeping
  if (power_is_sleeping()) {
    delay(100);  // Long sleep when display off
    return;
  }

  // ===== AWAKE MODE - Full processing =====

  lv_task_handler();

#ifdef DIRECT_RENDER_MODE
#if defined(CANVAS) || defined(RGB_PANEL) || defined(DSI_PANEL)
  gfx->flush();
#else
  gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#endif
#else
#ifdef CANVAS
  gfx->flush();
#endif
#endif

  ui_tick();
  rtc_tick();
  brightness_update();

  // Update battery less frequently (every 5 seconds instead of every loop)
  if (now - lastBatteryUpdate >= 5000) {
    battery_update();
    lastBatteryUpdate = now;
  }

  // Minimal delay for responsiveness
  delay(1);
}
