#include "v1_config.h"
#include <ESPAsyncWebServer.h>
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/actions.h"
#include "tft_v2.h"
#include <vector>
#include "wifi.h"
#include "v1_packet.h"
#include "math.h"
#include "time.h"

std::string v1LogicMode = "";
std::string prioAlertFreq = "START";
const char* tableFreqs[MAX_ALERTS];
const char* frequency_ptrs[MAX_ALERTS];
const char* direction_ptrs[MAX_ALERTS]; 
int alertCount = 0;
int prio_bars = 0;
int locationCount = 0;
bool bt_connected, showAlertTable, kAlert, xAlert, kaAlert, laserAlert, arrowPrioFront, arrowPrioSide, arrowPrioRear;

double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    double lat1_rad = lat1 * M_PI / 180.0;
    double lon1_rad = lon1 * M_PI / 180.0;
    double lat2_rad = lat2 * M_PI / 180.0;
    double lon2_rad = lon2 * M_PI / 180.0;

    double delta_lat = lat2_rad - lat1_rad;
    double delta_lon = lon2_rad - lon1_rad;

    // Haversine formula
    double a = sin(delta_lat / 2) * sin(delta_lat / 2) +
               cos(lat1_rad) * cos(lat2_rad) * sin(delta_lon / 2) * sin(delta_lon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double distance = EARTH_RADIUS_KM * c;

    return distance; // this is in km
}

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

extern "C" bool get_var_useDefaultV1Mode() {
    return settings.useDefaultV1Mode;
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
        LV_LOG_WARN("No active input device!");
        return;
    }

    lv_dir_t gesture = lv_indev_get_gesture_dir(indev);

    if(code == LV_EVENT_LONG_PRESSED) {
        LV_LOG_INFO("requesting manual lockout via long press");
        long_press_detected = true;

        if (gpsAvailable) {
            LockoutEntry thisLockout;            
            thisLockout.timestamp = gpsData.rawTime;
            thisLockout.latitude = gpsData.latitude;
            thisLockout.longitude = gpsData.longitude;
            thisLockout.entryType = "manual";

            Serial.printf("%u: Locking out lat: %f, lon: %f\n", thisLockout.timestamp, thisLockout.latitude, thisLockout.longitude);
            show_popup("Lockout Stored");
        }
    }
    else if(code == LV_EVENT_CLICKED && !long_press_detected) {
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
        else {
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
    else if(code == LV_EVENT_RELEASED) {
        long_press_detected = false;
    }
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

extern "C" bool get_var_wifiEnabled() {
    return settings.enableWifi;
}

extern "C" void set_var_wifiEnabled(bool enable) {
    if (enable) {
        settings.enableWifi = true;
        Serial.println("WiFi enabled and attempting to connect...");
        wifiScan();
    } else {
        settings.enableWifi = false;
        Serial.println("Disabling WiFi...");

        if (WiFi.getMode() == WIFI_MODE_STA) {
            WiFi.disconnect(true, true);
            Serial.println("Disconnected from WiFi network.");
        } 
        else if (WiFi.getMode() == WIFI_MODE_AP) {
            WiFi.softAPdisconnect(true);
            Serial.println("Access Point disabled.");
        }

        WiFi.mode(WIFI_MODE_NULL);
        Serial.println("WiFi module powered down.");
    }
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
    return value ? globalConfig.defaultMode : globalConfig.mode;
}

extern "C" const char *get_var_prio_alert_freq() {
    return prioAlertFreq.c_str();
}

extern "C" void set_var_prio_alert_freq(const char *value) {
    prioAlertFreq = value;
}

extern "C" void set_var_alertCount(int value) {
    alertCount = value;
}

extern "C" int get_var_alertCount() {
    return alertCount;
}

void set_var_frequencies(const std::vector<AlertTableData>& alertDataList) {
    static char alert_frequencies[MAX_ALERTS][16];
    static char alert_directions[MAX_ALERTS][16];

    int index = 0;
    for (const auto& alertData : alertDataList) {
        for (int i = 0; i < alertData.freqCount && index < MAX_ALERTS; i++) {
            snprintf(alert_frequencies[index], sizeof(alert_frequencies[index]), "%.3f", alertData.frequencies[i]);
            frequency_ptrs[index] = alert_frequencies[index];
            snprintf(alert_directions[index], sizeof(alert_directions[index]), "%s", alertData.direction[i].c_str());
            direction_ptrs[index] = alert_directions[index];

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
    return kAlert;
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

void displayTest() {
    // TODO: generate more synthetic packets as 31 paints the arrow/bars/band but the alert table doesn't match 1:1
    std::string packets[] = {"AAD6EA430713291D21858800E8AB", "AAD8EA31095B1F38280C0000E7AB", "AAD6EA4307235E569283240000AB", "AAD6EA430733878CB681228030AB"};
    //std::string packets[] = {"AAD8EA31095B5B0724248CCC5457AB", "AAD8EA310906060F24248CCC54B5AB"};

    for (const std::string& packet : packets) {
      //unsigned long decodeLoopStart = millis();
      PacketDecoder decoder(packet); 
      decoder.decode(settings.lowSpeedThreshold, 20);
      //Serial.printf("Decode loop: %lu\n", millis() - decodeLoopStart);
      delay(50);
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