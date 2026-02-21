#include "bluetooth.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include "mbedtls/base64.h"
#include "HWCDC.h"
#include "rtc_clock.h"
#include "notification_ui.h"
#include "power.h"
#include <eez/flow/flow.h>
#include "ui/WizWatch/src/ui/vars.h"

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

// Decode base64 from atob() and convert Latin-1 to UTF-8
static String decodeAtob(const String &b64) {
    size_t olen;
    unsigned char decoded[256];
    int ret = mbedtls_base64_decode(decoded, sizeof(decoded), &olen,
                                     (const unsigned char*)b64.c_str(), b64.length());
    if (ret != 0) return "?";

    // Convert Latin-1 to UTF-8, escaping JSON special chars
    String result;
    for (size_t i = 0; i < olen; i++) {
        uint8_t c = decoded[i];
        if (c < 0x20) continue;  // skip control chars
        if (c == '"') { result += "\\\""; continue; }
        if (c == '\\') { result += "\\\\"; continue; }
        if (c < 0x80) {
            result += (char)c;
        } else {
            // Latin-1 byte to 2-byte UTF-8
            result += (char)(0xC0 | (c >> 6));
            result += (char)(0x80 | (c & 0x3F));
        }
    }
    return result;
}

static void handleGBMessage(const String &json) {
    // Replace atob("...") with decoded UTF-8 string (Gadgetbridge base64-encodes non-ASCII text)
    String sanitized = json;
    int atobPos = 0;
    while ((atobPos = sanitized.indexOf("atob(\"", atobPos)) >= 0) {
        int endPos = sanitized.indexOf("\")", atobPos + 6);
        if (endPos < 0) break;
        String b64 = sanitized.substring(atobPos + 6, endPos);
        String decoded = decodeAtob(b64);
        sanitized = sanitized.substring(0, atobPos) + "\"" + decoded + "\"" + sanitized.substring(endPos + 2);
        atobPos += decoded.length() + 2;
    }

    // Convert \xNN hex escapes to actual bytes
    int pos = 0;
    while ((pos = sanitized.indexOf("\\x", pos)) >= 0) {
        if (pos + 3 < (int)sanitized.length()) {
            char c = (char)strtol(sanitized.substring(pos + 2, pos + 4).c_str(), nullptr, 16);
            sanitized = sanitized.substring(0, pos) + String(c) + sanitized.substring(pos + 4);
            pos++;
        } else {
            pos += 2;
        }
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, sanitized);
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

        // Show pop-up on watch display and keep screen on
        power_reset_inactivity();
        notification_ui_show(notif.src.c_str(), notif.title.c_str(), notif.body.c_str());
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
        if (callInfo.active) power_reset_inactivity();
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

// Send JSON to Gadgetbridge (watch -> phone)
static void sendGB(const String &json) {
    if (!deviceConnected) return;
    // Send empty line first to flush any pending REPL state (like real Bangle.js)
    String flush = "\r\n";
    pTxCharacteristic->setValue(flush.c_str());
    pTxCharacteristic->notify();
    delay(20);
    // Send the actual JSON
    String msg = json + "\r\n";
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
}

void bluetooth_init() {
    USBSerial.println("[BLE] Initializing Bluetooth...");

    // Name MUST start with "Bangle.js" for Gadgetbridge to recognize it
    BLEDevice::init("Bangle.js WizWatch");

    // Request larger MTU so messages fit in fewer packets
    BLEDevice::setMTU(256);

    // Enable "Just Works" bonding — stores keys in flash, no PIN required
    // Survives power cycles so Gadgetbridge can auto-reconnect
    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

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
    // Advertising interval: 1000-2000ms (power-efficient discovery)
    // Units are 0.625ms, so 0x640=1000ms, 0xC80=2000ms
    pAdvertising->setMinInterval(0x640);
    pAdvertising->setMaxInterval(0xC80);
    // Preferred connection interval once connected: 30-50ms
    pAdvertising->setMinPreferred(0x18);
    pAdvertising->setMaxPreferred(0x28);
    BLEDevice::startAdvertising();

    USBSerial.println("[BLE] Ready as 'Bangle.js WizWatch'");
}

void bluetooth_update() {
    // Process any received BLE data
    processRxBuffer();

    if (!deviceConnected && oldDeviceConnected) {
        // Clean up after disconnect
        rxBufLen = 0;
        rxLine = "";
        oldDeviceConnected = false;
        eez::flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_PHONE_CONNECTED_VAR, eez::Value(1));
        USBSerial.println("[BLE] Cleaning up connection...");
    }

    // Non-blocking delayed advertising restart
    static uint32_t disconnectTime = 0;
    if (!deviceConnected && !oldDeviceConnected && disconnectTime == 0) {
        disconnectTime = millis();
    }
    if (disconnectTime > 0 && !deviceConnected && (millis() - disconnectTime > 500)) {
        BLEDevice::startAdvertising();
        USBSerial.println("[BLE] Restarting advertising");
        disconnectTime = 0;
        oldDeviceConnected = false;  // ready for next connect
    }

    if (deviceConnected && !oldDeviceConnected) {
        // Fresh connection — clear buffers
        rxBufLen = 0;
        rxLine = "";
        disconnectTime = 0;
        oldDeviceConnected = true;
        eez::flow::setGlobalVariable(FLOW_GLOBAL_VARIABLE_PHONE_CONNECTED_VAR, eez::Value(0));
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

bool bluetooth_has_pending_data() {
    return rxBufLen > 0;
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

void bluetooth_find_phone(bool start) {
    if (start) {
        sendGB("{\"t\":\"findPhone\",\"n\":true}");
    } else {
        sendGB("{\"t\":\"findPhone\",\"n\":false}");
    }
    USBSerial.printf("[BLE] Find phone: %s\n", start ? "START" : "STOP");
}
