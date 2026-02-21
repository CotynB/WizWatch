#include "battery.h"
#include "ui/WizWatch/src/ui/vars.h"
#include <Arduino.h>
#include <eez/flow/flow.h>
#include <eez/core/value.h>
#include "HWCDC.h"
#include "power.h"

extern HWCDC USBSerial;

void battery_init() {
    battery_update();
}

int get_battery_level() {
    if (!PMU.isBatteryConnect()) return -1;
    return PMU.getBatteryPercent();
}

void battery_update() {
    static BatteryState lastState = (BatteryState)-1;
    static int lastPercent = -1;
    static int lastPlugged = -1;

    int level = get_battery_level();
    bool pluggedIn = PMU.isVbusIn();

    // Update battery icon state
    BatteryState state;
    if (level < 0) {
        // No battery connected — show full if plugged in, empty otherwise
        state = pluggedIn ? BATTERY_FULL : BATTERY_EMPTY;
    } else if (level > 66) {
        state = BATTERY_FULL;
    } else if (level > 33) {
        state = BATTERY_HALF;
    } else {
        state = BATTERY_EMPTY;
    }

    if (state != lastState) {
        eez::flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BATTERY_STATE, eez::Value((int)state));
        lastState = state;
    }

    // Update battery percent label
    if (level != lastPercent) {
        char buf[8];
        if (level < 0) {
            snprintf(buf, sizeof(buf), "--%%");
        } else {
            snprintf(buf, sizeof(buf), "%d%%", level);
        }
        eez::flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_BATTERY_PERCENT, eez::StringValue(buf));
        lastPercent = level;
    }

    // Update plugged icon (Vbus present = cable plugged in)
    int plugged = pluggedIn ? 1 : 0;
    if (plugged != lastPlugged) {
        eez::flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_PLUGGED_VAR, eez::Value(plugged));
        lastPlugged = plugged;
    }
}
