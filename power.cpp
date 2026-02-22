#include "power.h"
#include <Wire.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include "HWCDC.h"
#include "display.h"
#include "brightness.h"
#include "bluetooth.h"
#include "notification_ui.h"
#include "sd_card.h"

extern HWCDC USBSerial;
extern Arduino_GFX *gfx;

XPowersPMU PMU;
static bool sleeping = false;
static uint32_t lastActivityTime = 0;
static bool lightSleepConfigured = false;
static bool cpuSlowed = false;

#ifdef POWER_DEBUG
static PowerLogEntry _plog[POWER_LOG_CAPACITY];
static uint16_t _plog_head  = 0;
static uint16_t _plog_count = 0;
#endif

void power_init() {
    if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
        USBSerial.println("PMU init failed!");
        return;
    }

    // Enable ADC for battery monitoring
    PMU.enableBattDetection();
    PMU.enableVbusVoltageMeasure();
    PMU.enableBattVoltageMeasure();
    PMU.enableSystemVoltageMeasure();

    // Set charge target voltage
    PMU.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

    PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    PMU.clearIrqStatus();
    PMU.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ);
    lastActivityTime = millis();
}

void power_check_button() {
    if (PMU.getIrqStatus()) {
        if (PMU.isPekeyShortPressIrq()) {
            if (sleeping) {
                notification_ui_set_sleep_bg(false);
                power_wake();
            } else {
                power_sleep();
            }
        }
        PMU.clearIrqStatus();
    }
}

static void configure_light_sleep() {
    if (lightSleepConfigured) return;

    // Touch INT (GPIO 38) - active LOW on touch
    gpio_wakeup_enable(GPIO_NUM_38, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    // Timer wakeup every 500ms for BLE keepalive + PMU button poll
    esp_sleep_enable_timer_wakeup(500000);

    lightSleepConfigured = true;
}

void power_sleep() {
    sleeping = true;

    // Display off
    brightness_set(0);
    if (gfx) {
        gfx->displayOff();
    }

    // Unmount SD card
    sd_card_sleep();

    // BLE: stop advertising + request slow connection interval
    bluetooth_sleep();

    // Configure light sleep wakeup sources (once)
    configure_light_sleep();

    // If BLE disconnected: we'll use true light sleep in the main loop
    // If BLE connected: we'll use 40MHz + delay (BLE needs active CPU)
    if (!bluetooth_is_connected()) {
        // Light sleep mode - CPU will halt in the main loop
        setCpuFrequencyMhz(40);
    } else {
        // Connected sleep - BLE requires minimum 80MHz to maintain connection
        setCpuFrequencyMhz(80);
    }
}

void power_wake() {
    sleeping = false;
    lastActivityTime = millis();
    cpuSlowed = false;

    // Restore full CPU speed
    setCpuFrequencyMhz(240);

    // Display on — clear stale controller RAM before enabling brightness
    if (gfx) {
        gfx->displayOn();
        gfx->fillScreen(0x0000);  // overwrite old frame with black
    }
    display_set_brightness(51);
    delay(50);
    brightness_force_update();

    // Remount SD card
    sd_card_wake();

    // BLE: restart advertising + restore fast connection interval
    bluetooth_wake();
}

bool power_is_sleeping() {
    return sleeping;
}

void power_reset_inactivity() {
    lastActivityTime = millis();
    // Restore CPU speed immediately on activity
    if (cpuSlowed && !sleeping) {
        setCpuFrequencyMhz(240);
        cpuSlowed = false;
    }
}

void power_check_inactivity() {
    if (!sleeping && (millis() - lastActivityTime >= INACTIVITY_TIMEOUT_MS)) {
        power_sleep();
    }
}

void power_optimize_idle() {
    if (sleeping) return;

    uint32_t elapsed = millis() - lastActivityTime;

    if (!cpuSlowed && elapsed > 2000) {
        setCpuFrequencyMhz(80);
        cpuSlowed = true;
    }
    // Restore is handled in power_reset_inactivity() for immediate response
}

bool power_use_light_sleep() {
    return sleeping && !bluetooth_is_connected();
}

void power_log_sample() {
#ifdef POWER_DEBUG
    PowerLogEntry &e = _plog[_plog_head];
    e.ts      = millis();
    e.voltage = PMU.getBattVoltage();
    e.percent = PMU.isBatteryConnect() ? PMU.getBatteryPercent() : -1;
    e.cpu_mhz = (uint8_t)(getCpuFrequencyMhz() / 10);
    e.flags   = 0;
    if (sleeping)                              e.flags |= PFLAG_SLEEPING;
    if (bluetooth_is_connected())              e.flags |= PFLAG_BLE;
    if (PMU.isVbusIn())                        e.flags |= PFLAG_VBUS;
    if (sleeping && !bluetooth_is_connected()) e.flags |= PFLAG_LIGHTSLEEP;
    if (PMU.isCharging())                      e.flags |= PFLAG_CHARGING;

    _plog_head = (_plog_head + 1) % POWER_LOG_CAPACITY;
    if (_plog_count < POWER_LOG_CAPACITY) _plog_count++;
#endif
}

void power_log_flush() {
#ifdef POWER_DEBUG
    if (!USBSerial || _plog_count == 0) return;

    uint16_t start = (_plog_count < POWER_LOG_CAPACITY) ? 0 : _plog_head;

    USBSerial.printf("\n[PWR] %u samples — ts_ms,mV,pct,MHz,stage\n", _plog_count);
    for (uint16_t i = 0; i < _plog_count; i++) {
        const PowerLogEntry &e = _plog[(start + i) % POWER_LOG_CAPACITY];
        const char *stage =
            (e.flags & PFLAG_LIGHTSLEEP) ? "light_sleep" :
            (e.flags & PFLAG_SLEEPING)   ? "ble_sleep"   : "awake";
        USBSerial.printf("%lu,%.0f,%d,%d,%s%s\n",
            e.ts, e.voltage,
            e.percent, (int)e.cpu_mhz * 10, stage,
            (e.flags & PFLAG_CHARGING) ? ",chg" : "");
    }
    USBSerial.println("[PWR END]");

    _plog_count = 0;
    _plog_head  = 0;
#endif
}
