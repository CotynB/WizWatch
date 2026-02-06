#include "display.h"
#include "HWCDC.h"

extern HWCDC USBSerial;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);

Arduino_GFX *gfx = new Arduino_CO5300(bus, LCD_RESET,
                                      0, LCD_WIDTH, LCD_HEIGHT,
                                      22, 0, 0, 0);

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
#ifndef DIRECT_RENDER_MODE
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
#endif
  lv_disp_flush_ready(disp);
}

void rounder_event_cb(lv_event_t *e) {
  lv_area_t *area = (lv_area_t *)lv_event_get_param(e);
  area->x1 = (area->x1 >> 1) << 1;
  area->y1 = (area->y1 >> 1) << 1;
  area->x2 = ((area->x2 >> 1) << 1) + 1;
  area->y2 = ((area->y2 >> 1) << 1) + 1;
}

void display_init() {
  if (!gfx->begin()) {
    USBSerial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);

  // Set default brightness to 20% for power saving
  display_set_brightness(51);  // 20% brightness as default
}

void display_set_brightness(uint8_t brightness) {
  // Safety check - ensure gfx is valid
  if (!gfx) {
    return;
  }

  // Cast to CO5300 to access brightness control
  Arduino_CO5300 *co5300 = static_cast<Arduino_CO5300*>(gfx);
  co5300->setBrightness(brightness);
}
