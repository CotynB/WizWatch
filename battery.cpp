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
    USBSerial.println("[BATTERY] Initializing - setting state to FULL (2)");
    eez::flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BATTERY_STATE, eez::Value((int)BATTERY_FULL));

    // Verify it was set
    int readBack = eez::flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_BATTERY_STATE).getInt();
    USBSerial.print("[BATTERY] State set to: ");
    USBSerial.println(readBack);
}

int get_battery_level() {
    // TODO: Read actual battery level from ADC/fuel gauge
    // For now, return 100% (full)
    return 100;
}

void battery_update() {
    int level = get_battery_level();

    BatteryState state;
    if (level > 66) {
        state = BATTERY_FULL;
    } else if (level > 33) {
        state = BATTERY_HALF;
    } else {
        state = BATTERY_EMPTY;
    }

    // Set EEZ Flow global variable
    static BatteryState lastState = (BatteryState)-1;
    if (state != lastState) {
        USBSerial.print("[BATTERY] Updating state from ");
        USBSerial.print((int)lastState);
        USBSerial.print(" to ");
        USBSerial.println((int)state);
        lastState = state;
    }
    eez::flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BATTERY_STATE, eez::Value((int)state));
}
