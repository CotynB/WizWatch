#include "power.h"
#include "pin_config.h"
#include <Wire.h>
#include <XPowersLib.h>
#include "HWCDC.h"
#include "display.h"
#include "brightness.h"
#include "bluetooth.h"
#include "notification_ui.h"

extern HWCDC USBSerial;
extern Arduino_GFX *gfx;

XPowersPMU PMU;
static bool sleeping = false;
static uint32_t lastActivityTime = 0;

void power_init() {
    if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
        USBSerial.println("PMU init failed!");
        return;
    }

    USBSerial.println("PMU initialized");
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

void power_sleep() {
    USBSerial.println("Going to sleep");
    sleeping = true;

    brightness_set(0);
    if (gfx) {
        gfx->displayOff();  // AMOLED panel truly off — saves power
    }
    bluetooth_sleep();
    setCpuFrequencyMhz(80);

    USBSerial.println("Sleep mode active");
}

void power_wake() {
    sleeping = false;
    lastActivityTime = millis();

    setCpuFrequencyMhz(240);

    if (gfx) {
        gfx->displayOn();
        delay(50);
    }

    display_set_brightness(51);
    delay(50);
    brightness_update();
    bluetooth_wake();
}

bool power_is_sleeping() {
    return sleeping;
}

void power_reset_inactivity() {
    lastActivityTime = millis();
}

void power_check_inactivity() {
    if (!sleeping && (millis() - lastActivityTime >= INACTIVITY_TIMEOUT_MS)) {
        USBSerial.println("Inactivity timeout - going to sleep");
        power_sleep();
    }
}
