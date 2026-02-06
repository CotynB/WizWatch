// Include EEZ Studio generated UI files for compilation
// This ensures Arduino compiles the deeply nested .cpp files

#define EEZ_FOR_LVGL  // Enable EEZ Flow mode

#include "ui/WizWatch/src/ui/ui.cpp"
#include "ui/WizWatch/src/ui/screens.cpp"
#include "ui/WizWatch/src/ui/styles.cpp"
#include "ui/WizWatch/src/ui/vars.cpp"  // Native variables

// Note: images.cpp not included - using SD card instead
// Stub images array is defined below

#include "ui/WizWatch/src/ui/images.h"

#ifdef __cplusplus
extern "C" {
#endif

const ext_img_desc_t images[6] = {
    { "fond", nullptr },
    { "empty", nullptr },
    { "half_full", nullptr },
    { "full", nullptr },
    { "setttings_icon", nullptr },
    { "home_icon", nullptr },
};

#ifdef __cplusplus
}
#endif
