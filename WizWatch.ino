#include <Wire.h>
#include <Arduino.h>
#include "pin_config.h"
#include <lvgl.h>
#include "lv_conf.h"
#include "HWCDC.h"
#include <WiFi.h>
#include <esp_sleep.h>

#include "display.h"
#include "touch.h"
#include "rtc_clock.h"
#include "sd_card.h"
#include "battery.h"
#include "brightness.h"
#include "power.h"
#include "bluetooth.h"
#include "notification_ui.h"

// EEZ Studio generated UI
#include "ui/WizWatch/src/ui/ui.h"
#include "ui/WizWatch/src/ui/vars.h"
#include "ui/WizWatch/src/ui/screens.h"

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

  // USBSerial.begin(115200);  // Commented out for power saving
  // USBSerial.println("WizWatch starting...");

  // Disable WiFi for power saving (keep BLE for phone pairing)
  WiFi.mode(WIFI_OFF);
  USBSerial.println("WiFi disabled for power saving");

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

    // Wire Find Phone button — toggle ring on press/release
    static bool findPhoneActive = false;
    lv_obj_add_event_cb(objects.find_phone_btt, [](lv_event_t *e) {
        lv_event_code_t code = lv_event_get_code(e);
        bool *active = (bool *)lv_event_get_user_data(e);
        if (code == LV_EVENT_CLICKED) {
            *active = !*active;
            bluetooth_find_phone(*active);
        }
    }, LV_EVENT_CLICKED, &findPhoneActive);

    // Initialize battery and brightness after UI/Flow system is ready
    battery_init();
    brightness_init();

    // Set initial time to avoid flicker from default value
    rtc_update_display();
  }

  // Initialize notification overlay (after UI)
  notification_ui_init();

  // Initialize Bluetooth last (after UI is ready)
  bluetooth_init();

  USBSerial.println("Ready");
}

void loop() {
  static uint32_t lastBatteryUpdate = 0;
  uint32_t now = millis();

  // Always check power button (wake source)
  power_check_button();

  // ===== SLEEP MODE =====
  if (power_is_sleeping()) {
    if (power_use_light_sleep()) {
      // True light sleep — CPU halts, ~0.8-2mA
      esp_light_sleep_start();
      delay(5);  // Let BLE/FreeRTOS tasks process after wakeup

      // If woke from GPIO (touch), set flag since edge ISR misses during light sleep
      if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) {
        FT3168->IIC_Interrupt_Flag = true;
      }
    } else {
      // Connected sleep — CPU at 40MHz, BLE stays active
      delay(100);
    }

    // Check wake sources
    if (touch_has_activity()) {
      notification_ui_set_sleep_bg(false);
      power_wake();
    } else if (bluetooth_has_pending_data()) {
      notification_ui_set_sleep_bg(true);
      power_wake();
    }
    return;
  }

  // ===== AWAKE MODE =====

  // Scale CPU down when idle, up on touch (before LVGL processes)
  power_optimize_idle();

  lv_task_handler();

  if (power_is_sleeping()) return;

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
  bluetooth_update();  // Handle BLE connections
  power_check_inactivity();  // Auto-sleep after 30s of no touch

  // Update battery every 5 seconds
  if (now - lastBatteryUpdate >= 5000) {
    battery_update();
    lastBatteryUpdate = now;
  }

  // Use longer delay when LVGL is idle (no UI changes) to reduce CPU wake-ups
  uint32_t idleTime = lv_display_get_inactive_time(disp);
  if (idleTime > 1000) {
    delay(33);  // ~30 FPS idle polling
  } else {
    delay(1);   // Full speed during UI activity
  }
}
