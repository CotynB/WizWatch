#include "battery.h"
#include "ui/WizWatch/src/ui/vars.h"
#include <Arduino.h>
#include <eez/flow/flow.h>
#include "HWCDC.h"

extern HWCDC USBSerial;

// TODO: Add actual battery monitoring hardware
// For now, just return a fixed value for testing

void battery_init() {
    // Initialize battery monitoring hardware
    // Set initial battery state to FULL for testing
    eez::flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BATTERY_STATE, eez::Value((int)BATTERY_FULL));
}

int get_battery_level() {
    // TODO: Read actual battery level from ADC/fuel gauge
    // For now, return 100% (full)
    return 100;
}

void battery_update() {
    static BatteryState lastState = (BatteryState)-1;

    int level = get_battery_level();

    BatteryState state;
    if (level > 66) {
        state = BATTERY_FULL;
    } else if (level > 33) {
        state = BATTERY_HALF;
    } else {
        state = BATTERY_EMPTY;
    }

    // Only update if state changed to avoid unnecessary global variable writes
    if (state != lastState) {
        eez::flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BATTERY_STATE, eez::Value((int)state));
        lastState = state;
    }
}
