#include "v1_config.h"
#include <ESPAsyncWebServer.h>
#include "ui/ui.h"
#include "ui/actions.h"
#include "ui/blinking.h"
#include "utils.h"
#include "wifi.h"
#include "v1_packet.h"
#include "math.h"
#include "time.h"
#include "ble.h"

std::string v1LogicMode = "";
std::string prioAlertFreq = "START";
std::string photoType = "";
const char* tableFreqs[MAX_ALERTS];
const char* frequency_ptrs[MAX_ALERTS];
const char* direction_ptrs[MAX_ALERTS]; 
int alertCount = 0;
int prio_bars = 0;
int alertTableSize = 0;
bool proxyConnected, bt_connected, showAlertTable, kAlert, xAlert, kaAlert, laserAlert, arrowPrioFront, arrowPrioSide, arrowPrioRear;
bool photoAlertPresent;

//int locationCount = 0;
/*
void checkProximityForMute(double currentLat, double currentLon) {
    if (locationCount == 0) return;

    for (int i = 0; i < locationCount; i++) {
        double latDiff = fabs(currentLat - savedLockoutLocations[i].latitude);
        double lonDiff = fabs(currentLon - savedLockoutLocations[i].longitude);

        // Quick bounding box check
        if (latDiff > LAT_OFFSET || lonDiff > LON_OFFSET) continue;

        // Precise Haversine distance check
        double distance = haversineDistance(
            savedLockoutLocations[i].latitude, savedLockoutLocations[i].longitude, currentLat, currentLon
        );

        if (distance <= MUTING_RADIUS_KM) {
            Serial.println("Auto-mute activated!");
            clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqMuteOn(), 7, false);
            return;
        }
    }
}
*/

extern "C" bool get_var_proxyConnected() {
    return proxyConnected;
}

extern "C" bool get_var_useProxy() {
    return settings.proxyBLE;
}

extern "C" void set_var_useProxy(bool value) {
    preferences.begin("settings", false);
    if (!value) {
        settings.proxyBLE = false;
        Serial.println("Enabling proxy"); 
    }
    else {
        settings.proxyBLE = true;
        Serial.println("Disabling proxy");
    }

    preferences.putBool("proxyBLE", settings.proxyBLE);
    preferences.end();
}

extern "C" bool get_var_wifiClientConnected() {
    return wifiClientConnected;
}

extern "C" void set_var_photoType(const char *value) {
    photoType = value;
}

extern "C" const char *get_var_photoType() {
    return photoType.c_str();
}

extern "C" void set_var_photoAlertPresent(bool value) {
    photoAlertPresent = value;
}

extern "C" bool get_var_photoAlertPresent() {
    return photoAlertPresent;
}

extern "C" bool get_var_blankDisplay() {
    return settings.turnOffDisplay;
}

extern "C" void set_var_blankDisplay(bool value) {
    preferences.begin("settings", false);
    if (!value) {
        settings.turnOffDisplay = false;
        Serial.println("Enabling display"); 
    }
    else {
        settings.turnOffDisplay = true;
        Serial.println("Disabling display");
    }

    preferences.putBool("turnOffDisplay", settings.turnOffDisplay);
    preferences.end();
}

extern "C" bool get_var_dispBTIcon() {
    return settings.onlyDisplayBTIcon;
}

extern "C" void set_var_dispBTIcon(bool value) {
    preferences.begin("settings", false);
    if (!value) {
        settings.onlyDisplayBTIcon = false;
        Serial.println("BT Icon disabled"); 
    }
    else {
        settings.onlyDisplayBTIcon = true;
        Serial.println("BT Icon enabled");
    }

    preferences.putBool("onlyDispBTIcon", settings.onlyDisplayBTIcon);
    preferences.end();
}

extern "C" bool get_var_colorBars() {
    return settings.colorBars; 
}

extern "C" void set_var_colorBars(bool value) {
    preferences.begin("settings", false);
    if (!value) {
        settings.colorBars = false;
        Serial.println("Color Bars disabled"); 
    }
    else {
        settings.colorBars = true;
        Serial.println("Color Bars enabled");
    }

    preferences.putBool("colorBars", settings.colorBars);
    preferences.end();
}

extern "C" bool get_var_muteToGray() {
    return settings.muteToGray;
}

extern "C" void set_var_muteToGray(bool value) {
    preferences.begin("settings", false);
    if (!value) {
        settings.muteToGray = false;
        Serial.println("Mute-to-Gray disabled");
    } else {
        settings.muteToGray = true;
        Serial.println("Mute-to-Gray enabled");
    }
    
    preferences.putBool("muteToGray", settings.muteToGray);
    preferences.end();
}

// This function is utilized to set the T4 display to NOT use the name of the mode in the upper-left
extern "C" bool get_var_useDefaultV1Mode() {
    return settings.useDefaultV1Mode;
}

extern "C" void set_var_useDefaultV1Mode(bool value) {
    preferences.begin("settings", false);
    if (!value) {
        settings.useDefaultV1Mode = false;
        Serial.println("Default Mode disabled"); 
    }
    else {
        settings.useDefaultV1Mode = true;
        Serial.println("Default Mode enabled");
    }

    preferences.putBool("useDefMode", settings.useDefaultV1Mode);
    preferences.end();
}

extern "C" bool get_var_customFreqEnabled() {
    return (!globalConfig.customFreqEnabled && settings.useDefaultV1Mode && bt_connected); // default state is disable for customFreqEnabled
}

extern "C" void disconnectCurrentDevice() {
    if (pClient && pClient->isConnected()) {
        LV_LOG_INFO("Disconnecting current device...");
        pClient->disconnect();
        pClient = nullptr;
    }
}

extern "C" bool get_var_v1clePresent() {
    return v1le;
}

extern "C" void set_var_usev1cle(bool switch_state) {
    preferences.begin("settings", false);
    if (!switch_state) {
        settings.useV1LE = false;
        Serial.println("V1 CLE disabled; switching to V1");
    } else {
        settings.useV1LE = true;
        Serial.println("V1 CLE enabled; switching from V1");
    }
    
    disconnectCurrentDevice();

    preferences.putBool("useV1LE", settings.useV1LE);
    preferences.end();
}

extern "C" bool get_var_usev1cle() {
    return settings.useV1LE;
}

extern "C" void main_press_handler(lv_event_t * e) {

    static bool long_press_detected = false;
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t * indev = lv_indev_get_act();

    if (!indev) {
        //LV_LOG_WARN("No active input device!");
        return;
    }

    lv_dir_t gesture = lv_indev_get_gesture_dir(indev);

    if (code == LV_EVENT_LONG_PRESSED) {
        LV_LOG_INFO("long press detected");
        long_press_detected = true;
        Serial.println("long press detected");
        uint8_t newMode;

        switch (globalConfig.rawMode) {
            case MODE_ALL_BOGEYS:
                newMode = MODE_ALL_BOGEYS + 1;
                break;
            case MODE_LOGIC:
                newMode = MODE_LOGIC + 1;
                break;
            case MODE_ADVLOGIC:
                newMode = MODE_ALL_BOGEYS;
                break;
        }

        Serial.printf("Changing mode from %d to: %d\n", globalConfig.rawMode, newMode);
        if (bt_connected && clientWriteCharacteristic && !alertPresent) {
            clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqChangeMode(newMode), 8, false);
          }
        show_popup("Changing Mode...");

        /* // TODO: figure out what to do here
        if (gpsAvailable) {
            LockoutEntry thisLockout;
            if (xSemaphoreTake(gpsDataMutex, portMAX_DELAY)) {
                thisLockout.timestamp = gpsData.rawTime;
                thisLockout.latitude = gpsData.latitude;
                thisLockout.longitude = gpsData.longitude;
                thisLockout.entryType = "manual";

                xSemaphoreGive(gpsDataMutex);
            }

            Serial.printf("%u: Locking out lat: %f, lon: %f\n", thisLockout.timestamp, thisLockout.latitude, thisLockout.longitude);
            show_popup("Lockout Stored");
        }
        */
    }
    else if (code == LV_EVENT_CLICKED && !long_press_detected) {
        if (gesture == LV_DIR_LEFT) {
            LV_LOG_INFO("Swipe Left - Go to Settings");
            lv_scr_load_anim(objects.settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, false);
            loadScreen(SCREEN_ID_SETTINGS);
        } 
        else if (gesture == LV_DIR_RIGHT) {
            LV_LOG_INFO("Swipe Right - Go to Main Screen");
            lv_scr_load_anim(objects.main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, false);
            loadScreen(SCREEN_ID_MAIN);
        }
        else if (alertPresent) {
            LV_LOG_INFO("requesting mute via short press");
            Serial.println("requesting mute via short press");
            if (clientWriteCharacteristic && bt_connected) {
                clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqMuteOn(), 7, false);
                delay(20);
                show_popup("V1 Muted");
            } else {
                LV_LOG_WARN("BLE characteristic is NULL, cannot send mute command");
            }
        }
    }
    else if (code == LV_EVENT_RELEASED) {
        long_press_detected = false;
    }
}

extern "C" bool get_var_alertPresent() {
    return alertPresent;
}

extern "C" unsigned long getMillis() {
    return millis();
}

extern "C" int getWifiRSSI() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.RSSI();
    } else {
        return 0;
    }
}

extern "C" int getBluetoothSignalStrength() {
    if (!pClient || !pClient->isConnected()) {
        LV_LOG_WARN("BLE Client is null or not connected, returning RSSI as 0");
        return 0;
    }

    int rssi = pClient->getRssi();
    if (rssi == 0) LV_LOG_WARN("Received an invalid RSSI value");

    return rssi;
}

extern "C" bool get_var_wifiConnected() {
    return wifiConnected;
}

extern "C" uint8_t get_var_brightness() {
    uint8_t value = amoled.getBrightness();
    return value;
}

extern "C" void set_var_brightness(uint8_t value) {
    amoled.setBrightness(value);
}

extern "C" bool get_var_localWifi() {
    return localWifiStarted;
}

extern "C" bool get_var_wifiEnabled() {
    return settings.enableWifi;
}

extern "C" void set_var_wifiEnabled(bool enable) {
    preferences.begin("settings", false);
    if (enable) {
        settings.enableWifi = true;
        Serial.println("WiFi enabled and attempting to connect...");
        if (wifiScanTaskHandle == NULL) {
            xTaskCreate(wifiScanTask, "wifiScanTask", 4096, NULL, 1, &wifiScanTaskHandle);
        } else {
            Serial.println("WiFi scan task already running, skipping...");
        }
    } else {
       stopWifi();  
    }
    preferences.putBool("enableWifi", settings.enableWifi);
    preferences.end();
}

extern "C" const char *get_var_ssid() {
    static char ssid_buf[33];

    if (WiFi.getMode() == WIFI_MODE_AP) {
        strncpy(ssid_buf, settings.localSSID.c_str(), sizeof(ssid_buf) - 1);
    } else if (WiFi.getMode() == WIFI_MODE_STA) {
        strncpy(ssid_buf, WiFi.SSID().c_str(), sizeof(ssid_buf) - 1);
    }

    ssid_buf[sizeof(ssid_buf) - 1] = '\0';
    return ssid_buf;
}

extern "C" const char *get_var_password() {
    return settings.localPW.c_str();
}

extern "C" const char *get_var_ipAddress() {
    static char ip_buf[16];

    if (WiFi.getMode() == WIFI_MODE_AP) {
        strncpy(ip_buf, WiFi.softAPIP().toString().c_str(), sizeof(ip_buf) - 1);
    } else if (WiFi.getMode() == WIFI_MODE_STA) {
        strncpy(ip_buf, WiFi.localIP().toString().c_str(), sizeof(ip_buf) - 1);
    } else {
        strncpy(ip_buf, "0.0.0.0", sizeof(ip_buf) - 1);
    }

    ip_buf[sizeof(ip_buf) - 1] = '\0';
    return ip_buf;
}

extern "C" bool get_var_bt_connected() {
    return bt_connected;
}

extern "C" void set_var_bt_connected(bool value) {
    bt_connected = value;
}

extern "C" int get_var_prioBars() {
    return prio_bars;
}

extern "C" void set_var_prioBars(int value) {
    prio_bars = value;
}

extern "C" const char *get_var_logicmode(bool value) {
    //return value ? globalConfig.defaultMode : globalConfig.mode;
    const char* result = value ? globalConfig.defaultMode : globalConfig.mode;
    if (bt_connected) {
        return result ? result : "U";
    }
    else return "L";
}

extern "C" const char *get_var_prio_alert_freq() {
    return prioAlertFreq.c_str();
}

extern "C" void set_var_prio_alert_freq(const char *value) {
    prioAlertFreq = value;
}

extern "C" void set_var_alertTableSize(int value) {
    alertTableSize = value;
}

extern "C" int get_var_alertTableSize() {
    return alertTableSize;
}

extern "C" void set_var_alertCount(int value) {
    alertCount = value;
}

extern "C" uint8_t get_var_alertCount() {
    return alertCount;
}

void set_var_frequencies(const std::vector<AlertTableData>& alertDataList) {
    static char alert_frequencies[MAX_ALERTS][16];
    static char alert_directions[MAX_ALERTS][16];

    int index = 0;
    for (const auto& alertData : alertDataList) {
        constexpr int MAX_FREQ_COUNT = 4;
        //Serial.printf("Alert #%d - alertCount: %d, freqCount: %d\n", index + 1, alertData.alertCount, alertData.freqCount);

        for (int i = 0; i < alertData.freqCount && i < MAX_FREQ_COUNT && index < MAX_ALERTS; i++) {
            snprintf(alert_frequencies[index], sizeof(alert_frequencies[index]), "%.3f", alertData.frequencies[i]);
            frequency_ptrs[index] = alert_frequencies[index];

            strncpy(alert_directions[index], alertData.direction[i].c_str(), sizeof(alert_directions[index]) - 1);
            alert_directions[index][sizeof(alert_directions[index]) - 1] = '\0';
            direction_ptrs[index] = alert_directions[index];

            //Serial.printf(" -> Freq[%d]: %s, Dir[%d]: %s\n", i, alert_frequencies[index], i, alert_directions[index]);

            index++;
        }
    }

    if (index < MAX_ALERTS) {
        frequency_ptrs[index] = nullptr;
        direction_ptrs[index] = nullptr;
    }
}

extern "C" const char** get_var_frequencies() {
    return frequency_ptrs;
}

extern "C" const char** get_var_directions() {
    return direction_ptrs;
}

// do I need this if I use an extern? just implement a getter?
extern "C" void set_var_alertTableFreqs(const char* values[], int count) {
    if (count > MAX_ALERTS) { count = MAX_ALERTS; }

    for (int i = 0; i < count; ++i) {
        tableFreqs[i] = values[i];
    }
    alertCount = count;
}

extern "C" void set_var_arrowPrioFront(bool value) {
    arrowPrioFront = value;
}

extern "C" bool get_var_arrowPrioFront() {
    return arrowPrioFront;
}

extern "C" void set_var_arrowPrioSide(bool value) {
    arrowPrioSide = value;
}

extern "C" bool get_var_arrowPrioSide() {
    return arrowPrioSide;
}

extern "C" void set_var_arrowPrioRear(bool value) {
    arrowPrioRear = value;
}

extern "C" bool get_var_arrowPrioRear() {
    return arrowPrioRear;
}

extern "C" bool get_var_muted() {
    return muted;
}

extern "C" bool get_var_useImperial() {
    if (settings.unitSystem == METRIC) {
        return false;
    }
    return true;
}

extern "C" void set_var_useImperial(bool value) {
    preferences.begin("settings", false);
    if (value) {
        settings.unitSystem = IMPERIAL;
        Serial.println("Unit system set to Imperial");
        preferences.putString("unitSystem", "Imperial");
    } else {
        settings.unitSystem = METRIC;
        Serial.println("Unit system set to Metric");
        preferences.putString("unitSystem", "Metric");
    }

    preferences.end();
}

extern "C" int get_var_speedThreshold() {
    return settings.lowSpeedThreshold;
}

extern "C" void set_var_speedThreshold(int value) {
    preferences.begin("settings", false);
    settings.lowSpeedThreshold = value;
    Serial.printf("SilentRide threshold set to: %d\n", value);
    preferences.putInt("lowSpeedThres", settings.lowSpeedThreshold);
    preferences.end();
}

extern "C" bool get_var_showBogeys() {
    return settings.showBogeyCount;
}

extern "C" void set_var_showBogeys(bool value) {
    preferences.begin("settings", false);
    if (!value) {
        settings.showBogeyCount = false;
        Serial.println("Bogeys disabled"); 
    }
    else {
        settings.showBogeyCount = true;
        Serial.println("Bogeys enabled");
    }

    preferences.putBool("showBogeys", settings.showBogeyCount);
    preferences.end();
}

extern "C" void set_var_muted(bool value) {
    muted = value;
}

extern "C" bool get_showAlertTable() {
    return showAlertTable;
}

extern "C" void set_var_showAlertTable(bool value) {
    showAlertTable = value;
}

extern "C" void set_var_kAlert(bool value) {
    kAlert = value;
}

extern "C" bool get_var_kAlert() {
    return k_state.active;
}

extern "C" void set_var_xAlert(bool value) {
    xAlert = value;
}

extern "C" bool get_var_xAlert() {
    return xAlert;
}

extern "C" void set_var_kaAlert(bool value) {
    kaAlert = value;
}

extern "C" bool get_var_kaAlert() {
    return kaAlert;
}

extern "C" void set_var_laserAlert(bool value) {
    laserAlert = value;
}

extern "C" bool get_var_laserAlert() {
    return laserAlert;
}

extern "C" const char *get_var_lowspeedthreshold() {
    static char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", settings.lowSpeedThreshold);
    return buffer;
}

extern "C" bool get_var_gpsEnabled() {
    return settings.enableGPS && gpsAvailable;
}

extern "C" bool get_var_gpsAvailable() {
    return gpsAvailable;
}

void processingTask(void *pvParameters) {
    Serial.println("Processing Task Started");
    RadarPacket received;

    while (true) {
        // This blocks and uses 0% CPU until a packet arrives in the queue
        if (xQueueReceive(radarQueue, &received, portMAX_DELAY)) {
            //uint32_t start = millis();
            if (received.length == 0) {Serial.println("error, packet length of 0"); continue; }
            //Serial.printf("Received packet, len: %d, first byte: 0x%02X\n", received.length, received.data[0]);

            // Convert back to vector or pass the raw array to your decoder
            std::vector<uint8_t> vec(received.data, received.data + received.length);
            
            PacketDecoder decoder(vec);
            decoder.decode_v2(settings.lowSpeedThreshold, 20);
            //Serial.printf("Decode time: %lu ms\n", millis() - start);
        }
    }
}

void displayTestTask(void *pvParameters) {
    Serial.println("Test Task Started");
    const std::vector<std::vector<uint8_t>> syntheticPackets = {
        {0xAA, 0xD6, 0xEA, 0x43, 0x07, 0x13, 0x29, 0x1D, 0x21, 0x85, 0x88, 0x00, 0xE8, 0xAB},
        {0xAA, 0xD6, 0xEA, 0x43, 0x07, 0x23, 0x5E, 0x56, 0x92, 0x83, 0x24, 0x00, 0x00, 0xAB}, // bit 11 swap to 0x04 for photoRadar
        {0xAA, 0xD6, 0xEA, 0x43, 0x07, 0x33, 0x87, 0x8C, 0xB6, 0x81, 0x22, 0x80, 0x30, 0xAB},
        {0xAA, 0xD8, 0xEA, 0x31, 0x09, 0x4F, 0x00, 0x07, 0x28, 0x28, 0x10, 0x00, 0x00, 0x34, 0xAB}, // X band
        {0xAA, 0xD8, 0xEA, 0x31, 0x09, 0x4F, 0x4F, 0x3F, 0x22, 0x00, 0x50, 0x00, 0x35, 0x2A, 0xAB}, // Ka band - Prio + Blink
        {0xAA, 0xD8, 0xEA, 0x31, 0x09, 0x4F, 0x4F, 0x0F, 0x24, 0x24, 0x50, 0x00, 0x35, 0x20, 0xAB}, // K band
    };

    while (true) {
        for (const auto& rawVector : syntheticPackets) {
            RadarPacket packet;
            packet.length = rawVector.size();
            
            if (packet.length > 0 && packet.length <= 32) {
                memcpy(packet.data, rawVector.data(), packet.length);

                if (xQueueSend(radarQueue, &packet, 0) == pdPASS) {
                    //Serial.printf("Sent packet: %d bytes\n", packet.length);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
            } else {
                Serial.printf("Error: Synthetic packet has invalid length: %d\n", packet.length);
            }
        }
        // Pause between loops to let the display "breathe"
        vTaskDelay(pdMS_TO_TICKS(1200));
    }
}

/*
void saveSelectedConstants(const DisplayConstants& constants) {
  preferences.putBytes("selConstants", &constants, sizeof(DisplayConstants));
}

void loadSelectedConstants(DisplayConstants& constants) {
  preferences.getBytes("selConstants", &constants, sizeof(DisplayConstants));
}
*/