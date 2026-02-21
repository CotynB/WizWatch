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

void power_init() {
    if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
        USBSerial.println("PMU init failed!");
        return;
    }

    USBSerial.println("PMU initialized");

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
    USBSerial.println("Power button enabled");
    lastActivityTime = millis();
}

void power_check_button() {
    if (PMU.getIrqStatus()) {
        if (PMU.isPekeyShortPressIrq()) {
            USBSerial.println("Power button pressed!");
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
    USBSerial.println("Going to sleep");
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
        USBSerial.println("Sleep mode: light sleep (BLE disconnected)");
    } else {
        // Connected sleep - keep CPU running slowly for BLE
        setCpuFrequencyMhz(40);
        USBSerial.println("Sleep mode: low-power poll (BLE connected)");
    }
}

void power_wake() {
    sleeping = false;
    lastActivityTime = millis();
    cpuSlowed = false;

    // Restore full CPU speed
    setCpuFrequencyMhz(240);

    // Display on
    if (gfx) {
        gfx->displayOn();
        delay(50);
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
        USBSerial.println("Inactivity timeout - going to sleep");
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
