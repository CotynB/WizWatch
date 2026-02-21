#include "brightness.h"
#include "ui/WizWatch/src/ui/vars.h"
#include <eez/flow/flow.h>
#include "display.h"
#include "HWCDC.h"

extern HWCDC USBSerial;

static int lastBrightness = -1;

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
    lastBrightness = initialBrightness;
}

void brightness_set(int level) {
    if (level < 0) level = 0;
    if (level > 100) level = 100;

    uint8_t brightness_val = (level * 255) / 100;
    display_set_brightness(brightness_val);
}

void brightness_update() {
    int brightness = eez::flow::getGlobalVariable(FLOW_GLOBAL_VARIABLE_BRIGHTNESS).getInt();

    if (brightness != lastBrightness) {
        brightness_set(brightness);
        lastBrightness = brightness;
    }
}

void brightness_force_update() {
    lastBrightness = -1;  // Force next update to apply
    brightness_update();
}
