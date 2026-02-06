#include "power.h"
#include "pin_config.h"
#include <Wire.h>
#include <XPowersLib.h>
#include "HWCDC.h"
#include "display.h"
#include "brightness.h"

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

    // Step 1: Turn off display backlight (this is enough - screen goes black)
    brightness_set(0);
    USBSerial.println("Backlight OFF (screen black)");
    delay(10);

    // Step 2: Reduce CPU frequency for extra power saving
    setCpuFrequencyMhz(80);  // Reduce from 240MHz to 80MHz
    USBSerial.println("CPU reduced to 80MHz");

    // NOTE: We don't call gfx->displayOff() because it might not wake properly
    // Just turning backlight to 0 is sufficient - screen appears off

    USBSerial.println("Sleep mode active - press button to wake");
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

    // Then restore user's brightness from slider
    brightness_update();
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
