#pragma once

#include <Arduino.h>

// Max stored notifications
#define BT_MAX_NOTIFICATIONS 5

// Notification from phone
struct bt_notification_t {
    uint32_t id;
    String src;     // App name (e.g. "WhatsApp")
    String sender;
    String title;
    String body;
};

// Music playback info
struct bt_music_t {
    String artist;
    String album;
    String track;
    int duration;   // seconds
    int position;   // seconds
    bool playing;
};

// Weather info
struct bt_weather_t {
    int temp;       // Celsius
    int humidity;
    String txt;     // Description ("Sunny", "Cloudy", etc.)
    int code;       // Weather code
    bool valid;
};

// Call state
struct bt_call_t {
    String cmd;     // "incoming", "accept", "reject", "start", "end"
    String name;
    String number;
    bool active;
};

// Bluetooth initialization and control
void bluetooth_init();
void bluetooth_update();
void bluetooth_disconnect();
bool bluetooth_is_connected();

// Power management
void bluetooth_sleep();
void bluetooth_wake();
bool bluetooth_has_pending_data();  // True if BLE data waiting to be processed

// Data accessors
const bt_notification_t* bluetooth_get_latest_notification();
int bluetooth_get_notification_count();
const bt_music_t* bluetooth_get_music();
const bt_weather_t* bluetooth_get_weather();
const bt_call_t* bluetooth_get_call();

// Actions (watch -> phone)
void bluetooth_send_music_command(const char* cmd); // "play", "pause", "next", "previous", "volumeup", "volumedown"
void bluetooth_dismiss_notification(uint32_t id);
void bluetooth_answer_call();
void bluetooth_reject_call();
void bluetooth_find_phone(bool start);
