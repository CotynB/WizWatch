#include "rtc_clock.h"
#include <Wire.h>
#include "pin_config.h"
#include "HWCDC.h"
#include "ui/WizWatch/src/ui/screens.h"
#include "ui/WizWatch/src/ui/vars.h"  // For EEZ native variables

// EEZ Studio generated native variable functions
extern "C" {
  const char *get_var_rtc_time();
  void set_var_rtc_time(const char *value);
}

extern HWCDC USBSerial;

SensorPCF85063 rtc;
static uint32_t lastMillis = 0;

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

  // Initialize lastMillis to prevent immediate update
  lastMillis = millis();
}

void rtc_update_display() {
  RTC_DateTime datetime = rtc.getDateTime();

  // Debug: print raw values
  USBSerial.print("RTC Values - H:");
  USBSerial.print(datetime.getHour());
  USBSerial.print(" M:");
  USBSerial.print(datetime.getMinute());
  USBSerial.print(" D:");
  USBSerial.print(datetime.getDay());
  USBSerial.print(" M:");
  USBSerial.print(datetime.getMonth());
  USBSerial.print(" Y:");
  USBSerial.println(datetime.getYear());

  char buf[100];
  // Temporarily use simple format for testing
  snprintf(buf, sizeof(buf), "%02d:%02d",
           datetime.getHour(), datetime.getMinute());

  USBSerial.print("Formatted: ");
  USBSerial.println(buf);

  // Update EEZ native variable - Flow will read this automatically
  set_var_rtc_time(buf);
}

void rtc_tick() {
  uint32_t now = millis();

  // Update every 1000ms
  if (now - lastMillis >= 1000) {
    lastMillis = now;
    rtc_update_display();
  }
}
