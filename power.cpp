#include "power.h"
#include "pin_config.h"
#include <Wire.h>
#include <XPowersLib.h>
#include "HWCDC.h"
#include "display.h"
#include "brightness.h"
#include "bluetooth.h"

extern HWCDC USBSerial;
extern Arduino_GFX *gfx;

XPowersPMU PMU;
static bool sleeping = false;
static uint8_t last_brightness = 50;  // Store brightness before sleep

void power_init() {
    // Initialize PMU for power button
    if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
        USBSerial.println("PMU init failed!");
        return;
    }

    USBSerial.println("PMU initialized");

    // Disable all interrupts first
    PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

    // Clear any pending interrupts
    PMU.clearIrqStatus();

    // Enable only power button short press interrupt
    PMU.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ);

    USBSerial.println("Power button enabled");
}

void power_check_button() {
    // Check if power button was pressed
    if (PMU.getIrqStatus()) {
        // Check if it was the power key
        if (PMU.isPekeyShortPressIrq()) {
            USBSerial.println("Power button pressed!");

            // Toggle sleep mode
            if (sleeping) {
                power_wake();
            } else {
                power_sleep();
            }
        }

        // Clear the interrupt
        PMU.clearIrqStatus();
    }
}

void power_sleep() {
    USBSerial.println("Going to sleep - MAX POWER SAVE MODE");

    sleeping = true;

    // Step 1: Turn off display backlight
    brightness_set(0);
    USBSerial.println("Backlight OFF");

    // Step 2: Stop BLE advertising to save power
    bluetooth_sleep();

    // Step 3: Reduce CPU frequency
    setCpuFrequencyMhz(80);
    USBSerial.println("CPU reduced to 80MHz");

    USBSerial.println("Sleep mode active");
}

void power_wake() {
    sleeping = false;

    // Restore CPU frequency to full speed
    setCpuFrequencyMhz(240);

    // Wake display from power-save mode
    if (gfx) {
        gfx->displayOn();
        delay(50);
    }

    // Set minimum brightness first to wake display
    display_set_brightness(51);  // 20% as minimum
    delay(50);

    // Restore user's brightness from slider
    brightness_update();

    // Resume BLE advertising
    bluetooth_wake();
}

bool power_is_sleeping() {
    return sleeping;
}

void power_optimize_idle() {
    // If sleeping, skip most processing and save power
    if (sleeping) {
        delay(100);  // Longer delay when sleeping = less CPU wake-ups
        return;
    }

    // When awake, still optimize:
    // Allow brief CPU sleep between loop iterations
    delay(1);
}
