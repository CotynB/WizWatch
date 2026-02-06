#include "brightness.h"
#include "ui/WizWatch/src/ui/vars.h"
#include <eez/flow/flow.h>
#include "display.h"
#include "HWCDC.h"

extern HWCDC USBSerial;

void brightness_init() {
    // Set initial brightness from EEZ variable
    int initialBrightness = eez::flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS).getInt();

    // Safety check - if 0, set to 50% (medium brightness)
    if (initialBrightness == 0) {
        initialBrightness = 50;
        USBSerial.println("Brightness was 0, setting to 50%");
    }

    USBSerial.print("Brightness init - EEZ value: ");
    USBSerial.println(initialBrightness);

    brightness_set(initialBrightness);
}

void brightness_set(int level) {
    // Clamp brightness to 0-100 range
    if (level < 0) level = 0;
    if (level > 100) level = 100;

    // Map 0-100 to 0-255 range for display
    uint8_t brightness_val = (level * 255) / 100;

    // Use display wrapper function
    display_set_brightness(brightness_val);
}

void brightness_update() {
    static int lastBrightness = -1;

    // Read brightness from EEZ Flow global variable
    int brightness = eez::flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS).getInt();

    // Always update on wake, even if value hasn't changed
    brightness_set(brightness);
    lastBrightness = brightness;
}
