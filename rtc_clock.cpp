#include "rtc_clock.h"
#include <Wire.h>
#include "pin_config.h"
#include "HWCDC.h"

extern HWCDC USBSerial;

SensorPCF85063 rtc;
uint32_t lastMillis;

void rtc_init() {
  if (!rtc.begin(Wire, IIC_SDA, IIC_SCL)) {
    USBSerial.println("Failed to find PCF8563 - check your wiring!");
    while (1) delay(1000);
  }

  uint16_t year = 2025;
  uint8_t month = 2;
  uint8_t day = 2;
  uint8_t hour = 12;
  uint8_t minute = 48;
  uint8_t second = 41;
  rtc.setDateTime(year, month, day, hour, minute, second);
}

void rtc_update_label(lv_obj_t *label) {
  if (millis() - lastMillis > 1000) {
    lastMillis = millis();
    RTC_DateTime datetime = rtc.getDateTime();

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d\n%02d-%02d-%04d",
             datetime.getHour(), datetime.getMinute(),
             datetime.getDay(), datetime.getMonth(), datetime.getYear());

    lv_label_set_text(label, buf);
  }
}
