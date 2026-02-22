#pragma once

#include "pin_config.h"
#include <XPowersLib.h>

extern XPowersPMU PMU;

#define INACTIVITY_TIMEOUT_MS 20000  // 20 seconds
// #define POWER_DEBUG  // Enable serial power profiling

#define POWER_LOG_CAPACITY 512   // ~8 min at 1 sample/s

struct PowerLogEntry {
    uint32_t ts;        // millis()
    float    voltage;   // mV
    int8_t   percent;   // -1 if battery not detected
    uint8_t  cpu_mhz;   // MHz / 10  (240→24, 80→8, 40→4)
    uint8_t  flags;     // see PFLAG_* below
};
// flags bits
#define PFLAG_SLEEPING    0x01
#define PFLAG_BLE         0x02
#define PFLAG_VBUS        0x04
#define PFLAG_LIGHTSLEEP  0x08
#define PFLAG_CHARGING    0x10

void power_init();
void power_check_button();
void power_sleep();
void power_wake();
bool power_is_sleeping();
void power_optimize_idle();    // Call in loop to reduce CPU when idle
void power_reset_inactivity(); // Call on user activity (touch, notification)
void power_check_inactivity(); // Call in loop to auto-sleep
bool power_use_light_sleep();  // True if should use esp_light_sleep_start()
void power_log_sample();       // Capture snapshot → ring buffer
void power_log_flush();        // Dump buffer to serial if USB connected, then clear
