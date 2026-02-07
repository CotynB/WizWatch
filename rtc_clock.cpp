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

  // Time will be synced from phone via BLE

  // Initialize lastMillis to prevent immediate update
  lastMillis = millis();
}

void rtc_update_display() {
  static char last_buf[6] = {0};

  RTC_DateTime datetime = rtc.getDateTime();

  char buf[6];  // Reduced buffer size - only need "HH:MM\0"
  snprintf(buf, sizeof(buf), "%02d:%02d",
           datetime.getHour(), datetime.getMinute());

  // Only update if time has changed to avoid redundant EEZ variable writes
  if (strcmp(buf, last_buf) != 0) {
    strcpy(last_buf, buf);
    set_var_rtc_time(buf);
  }
}

void rtc_set_from_epoch(long epoch) {
  time_t ts = (time_t)epoch;
  struct tm *t = gmtime(&ts);
  if (!t) return;

  rtc.setDateTime(
    t->tm_year + 1900,
    t->tm_mon + 1,
    t->tm_mday,
    t->tm_hour,
    t->tm_min,
    t->tm_sec
  );

  USBSerial.printf("[RTC] Set to %04d-%02d-%02d %02d:%02d:%02d\n",
    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
    t->tm_hour, t->tm_min, t->tm_sec);

  rtc_update_display();
}

void rtc_tick() {
  uint32_t now = millis();

  // Update every 1000ms
  if (now - lastMillis >= 1000) {
    lastMillis = now;
    rtc_update_display();
  }
}
