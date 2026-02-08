#pragma once

#include <Arduino.h>

// Initialize the notification overlay (call after ui_init)
void notification_ui_init();

// Show a notification pop-up (slides down from top, auto-dismisses after 5s)
void notification_ui_show(const char* src, const char* title, const char* body);

// Dismiss the current notification (if visible)
void notification_ui_dismiss();
