#include "bluetooth.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include "HWCDC.h"
#include "rtc_clock.h"

extern HWCDC USBSerial;

// Nordic UART Service (NUS) UUIDs - used by Bangle.js protocol
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Device Information Service (DIS) - Gadgetbridge checks this
#define DIS_SERVICE_UUID                "180A"
#define DIS_MANUFACTURER_CHAR_UUID      "2A29"
#define DIS_MODEL_CHAR_UUID             "2A24"
#define DIS_FIRMWARE_CHAR_UUID          "2A26"
#define DIS_SOFTWARE_CHAR_UUID          "2A28"

// BLE objects
static BLEServer *pServer = nullptr;
static BLECharacteristic *pTxCharacteristic = nullptr;
static BLECharacteristic *pRxCharacteristic = nullptr;

// Connection state
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

// Thread-safe receive buffer (callback just appends, main loop processes)
#define RX_BUF_SIZE 512
static char rxBuf[RX_BUF_SIZE];
static volatile int rxBufLen = 0;

// Line buffer for processing
static String rxLine = "";

// Data stores
static bt_notification_t notifications[BT_MAX_NOTIFICATIONS];
static int notificationCount = 0;
static int notificationHead = 0; // circular index
static bt_music_t musicInfo;
static bt_weather_t weatherInfo;
static bt_call_t callInfo;

// Forward declarations
static void handleGBMessage(const String &json);
static void sendGB(const String &json);

// Server callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        USBSerial.println("[BLE] Phone connected!");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        USBSerial.println("[BLE] Phone disconnected");
    }
};

// Receive data from Gadgetbridge — just buffer it, process in main loop
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String raw = pCharacteristic->getValue();
        int len = raw.length();
        if (len == 0) return;

        // Copy into thread-safe buffer
        int space = RX_BUF_SIZE - rxBufLen;
        if (len > space) len = space;
        if (len > 0) {
            memcpy(&rxBuf[rxBufLen], raw.c_str(), len);
            rxBufLen += len;
        }
    }
};

// Process buffered BLE data (called from main loop)
static void processRxBuffer() {
    if (rxBufLen == 0) return;

    // Grab data from volatile buffer
    int len = rxBufLen;
    char tmp[RX_BUF_SIZE];
    memcpy(tmp, rxBuf, len);
    rxBufLen = 0;  // Reset buffer

    // Append to line buffer
    for (int i = 0; i < len; i++) {
        char c = tmp[i];
        if (c == '\n') {
            rxLine.trim();
            // Strip leading control characters (Espruino sends \x10 prefix)
            while (rxLine.length() > 0 && rxLine[0] < 0x20) {
                rxLine = rxLine.substring(1);
            }
            if (rxLine.length() > 0) {
                USBSerial.print("[BLE] Recv: ");
                USBSerial.println(rxLine);

                if (rxLine.startsWith("GB(") && rxLine.endsWith(")")) {
                    String json = rxLine.substring(3, rxLine.length() - 1);
                    handleGBMessage(json);
                }
                // Extract timestamp from setTime(epoch);E.setTimeZone(tz);...
                else if (rxLine.startsWith("setTime(")) {
                    int end = rxLine.indexOf(')');
                    if (end > 8) {
                        long ts = rxLine.substring(8, end).toInt();

                        // Extract timezone offset (hours) from E.setTimeZone(X.X)
                        float tz = 0;
                        int tzStart = rxLine.indexOf("setTimeZone(");
                        if (tzStart >= 0) {
                            int tzEnd = rxLine.indexOf(')', tzStart + 12);
                            if (tzEnd > tzStart + 12) {
                                tz = rxLine.substring(tzStart + 12, tzEnd).toFloat();
                            }
                        }

                        long localTs = ts + (long)(tz * 3600);
                        USBSerial.printf("[BLE] Time sync: %ld (UTC%+.1f)\n", ts, tz);
                        rtc_set_from_epoch(localTs);
                    }
                }
            }
            rxLine = "";
        } else {
            rxLine += c;
        }
    }
}

static void handleGBMessage(const String &json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json, DeserializationOption::Filter(JsonDocument()),
                                                DeserializationOption::NestingLimit(10));
    // If InvalidInput, retry allowing raw bytes (handles accented chars like é)
    if (err == DeserializationError::InvalidInput) {
        String sanitized = json;
        // Replace \xNN escape sequences with '?'
        int pos = 0;
        while ((pos = sanitized.indexOf("\\x", pos)) >= 0) {
            sanitized = sanitized.substring(0, pos) + "?" + sanitized.substring(pos + 4);
        }
        err = deserializeJson(doc, sanitized);
    }
    if (err) {
        USBSerial.print("[BLE] JSON parse error: ");
        USBSerial.println(err.c_str());
        return;
    }

    const char* type = doc["t"];
    if (!type) return;

    USBSerial.print("[BLE] Message type: ");
    USBSerial.println(type);

    // ---- Notification ----
    if (strcmp(type, "notify") == 0) {
        bt_notification_t &notif = notifications[notificationHead];
        notif.id     = doc["id"] | 0;
        notif.src    = doc["src"] | "Unknown";
        notif.sender = doc["sender"] | "";
        notif.title  = doc["title"] | "";
        notif.body   = doc["body"] | "";

        notificationHead = (notificationHead + 1) % BT_MAX_NOTIFICATIONS;
        if (notificationCount < BT_MAX_NOTIFICATIONS) notificationCount++;

        USBSerial.printf("[BLE] Notification from %s: %s\n",
            notif.src.c_str(), notif.title.c_str());
    }
    // ---- Dismiss notification ----
    else if (strcmp(type, "notify-") == 0) {
        uint32_t id = doc["id"] | 0;
        for (int i = 0; i < notificationCount; i++) {
            if (notifications[i].id == id) {
                notifications[i].id = 0;
                notifications[i].src = "";
                notifications[i].title = "";
                notifications[i].body = "";
                USBSerial.printf("[BLE] Dismissed notification %d\n", id);
                break;
            }
        }
    }
    // ---- Music info ----
    else if (strcmp(type, "musicinfo") == 0) {
        musicInfo.artist   = doc["artist"] | "";
        musicInfo.album    = doc["album"] | "";
        musicInfo.track    = doc["track"] | "";
        musicInfo.duration = doc["dur"] | 0;
        musicInfo.position = doc["c"] | 0;
        USBSerial.printf("[BLE] Music: %s - %s\n",
            musicInfo.artist.c_str(), musicInfo.track.c_str());
    }
    // ---- Music state ----
    else if (strcmp(type, "musicstate") == 0) {
        const char* state = doc["state"] | "pause";
        musicInfo.playing = (strcmp(state, "play") == 0);
        USBSerial.printf("[BLE] Music %s\n", musicInfo.playing ? "playing" : "paused");
    }
    // ---- Time sync ----
    else if (strcmp(type, "setTime") == 0) {
        long ts = doc["ts"] | 0;
        if (ts > 0) {
            USBSerial.printf("[BLE] Time sync: %ld\n", ts);
            // TODO: update RTC with timestamp
        }
    }
    // ---- Weather ----
    else if (strcmp(type, "weather") == 0) {
        weatherInfo.temp     = doc["temp"] | 0;
        weatherInfo.humidity = doc["hum"] | 0;
        weatherInfo.txt      = doc["txt"] | "";
        weatherInfo.code     = doc["code"] | 0;
        weatherInfo.valid    = true;
        USBSerial.printf("[BLE] Weather: %d°C %s\n",
            weatherInfo.temp, weatherInfo.txt.c_str());
    }
    // ---- Incoming call ----
    else if (strcmp(type, "call") == 0) {
        callInfo.cmd    = doc["cmd"] | "";
        callInfo.name   = doc["name"] | "";
        callInfo.number = doc["number"] | "";
        callInfo.active = (callInfo.cmd == "incoming" || callInfo.cmd == "start");
        USBSerial.printf("[BLE] Call: %s from %s\n",
            callInfo.cmd.c_str(), callInfo.name.c_str());
    }
    // ---- Find my watch ----
    else if (strcmp(type, "find") == 0) {
        bool on = doc["n"] | false;
        USBSerial.printf("[BLE] Find: %s\n", on ? "ON" : "OFF");
        // TODO: trigger vibration motor or screen flash
    }
    // ---- GPS query ----
    else if (strcmp(type, "is_gps_active") == 0) {
        sendGB("{\"t\":\"gps_power\",\"status\":false}");
    }
    else {
        USBSerial.print("[BLE] Unknown type: ");
        USBSerial.println(type);
    }
}

// Send a GB() response back to the phone
static void sendGB(const String &json) {
    if (!deviceConnected) return;
    String msg = "\x10" + json + "\n";
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
}

void bluetooth_init() {
    USBSerial.println("[BLE] Initializing Bluetooth...");

    // Name MUST start with "Bangle.js" for Gadgetbridge to recognize it
    BLEDevice::init("Bangle.js WizWatch");

    // Request larger MTU so messages fit in fewer packets
    BLEDevice::setMTU(256);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // --- Nordic UART Service ---
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // TX: Watch -> Phone (notify)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    // RX: Phone -> Watch (write)
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE
    );
    pRxCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    // Advertising interval: 20-40ms (fast discovery, ~1mA more than default)
    // Units are 0.625ms, so 0x20=20ms, 0x40=40ms
    pAdvertising->setMinInterval(0x20);
    pAdvertising->setMaxInterval(0x40);
    // Preferred connection interval once connected
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();

    USBSerial.println("[BLE] Ready as 'Bangle.js WizWatch'");
}

void bluetooth_update() {
    // Process any received BLE data
    processRxBuffer();

    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        USBSerial.println("[BLE] Restarting advertising");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
}

void bluetooth_disconnect() {
    if (deviceConnected) {
        pServer->disconnect(pServer->getConnId());
    }
}

bool bluetooth_is_connected() {
    return deviceConnected;
}

void bluetooth_sleep() {
    BLEDevice::getAdvertising()->stop();
    USBSerial.println("[BLE] Advertising stopped (sleep)");
}

void bluetooth_wake() {
    if (!deviceConnected) {
        BLEDevice::getAdvertising()->start();
        USBSerial.println("[BLE] Advertising resumed");
    }
}

// --- Data accessors ---

const bt_notification_t* bluetooth_get_latest_notification() {
    if (notificationCount == 0) return nullptr;
    int idx = (notificationHead - 1 + BT_MAX_NOTIFICATIONS) % BT_MAX_NOTIFICATIONS;
    return &notifications[idx];
}

int bluetooth_get_notification_count() {
    return notificationCount;
}

const bt_music_t* bluetooth_get_music() {
    return &musicInfo;
}

const bt_weather_t* bluetooth_get_weather() {
    return &weatherInfo;
}

const bt_call_t* bluetooth_get_call() {
    return &callInfo;
}

// --- Actions (watch -> phone via Bangle.js protocol) ---

void bluetooth_send_music_command(const char* cmd) {
    JsonDocument doc;
    doc["t"] = "music";
    doc["n"] = cmd;
    String json;
    serializeJson(doc, json);
    sendGB(json);
    USBSerial.printf("[BLE] Music cmd: %s\n", cmd);
}

void bluetooth_dismiss_notification(uint32_t id) {
    JsonDocument doc;
    doc["t"] = "notify-";
    doc["id"] = id;
    String json;
    serializeJson(doc, json);
    sendGB(json);

    // Also remove locally
    for (int i = 0; i < notificationCount; i++) {
        if (notifications[i].id == id) {
            notifications[i].id = 0;
            notifications[i].src = "";
            break;
        }
    }
}

void bluetooth_answer_call() {
    sendGB("{\"t\":\"call\",\"n\":\"ACCEPT\"}");
    callInfo.active = false;
}

void bluetooth_reject_call() {
    sendGB("{\"t\":\"call\",\"n\":\"REJECT\"}");
    callInfo.active = false;
}
