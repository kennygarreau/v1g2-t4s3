#include "v1_config.h"
#include <ESPAsyncWebServer.h>
#include "lvgl.h"
#include "ui/ui.h"
#include "tft_v2.h"
#include <vector>
#include "wifi.h"

std::string v1LogicMode = "";
std::string prioAlertFreq = "START";
const char* tableFreqs[MAX_ALERTS];
const char* frequency_ptrs[MAX_ALERTS];
const char* direction_ptrs[MAX_ALERTS]; 
int alertCount = 0;
int prio_bars = 0;
bool bt_connected, showAlertTable, kAlert, xAlert, kaAlert, laserAlert, arrowPrioFront, arrowPrioSide, arrowPrioRear;

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
    if (pClient == nullptr) {
        LV_LOG_WARN("BLE Client is null, returning RSSI as 0");
        return 0;  // Use 0 or another invalid value to indicate an error
    }

    if (!pClient->isConnected()) {
        LV_LOG_WARN("BLE Client is not connected, returning RSSI as 0");
        return 0;
    }

    int rssi = pClient->getRssi();

    if (rssi == 0) {
        LV_LOG_WARN("Received an invalid RSSI value");
        return 0;
    }

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
        startWifiAsync();
        //WiFi.mode(WIFI_MODE_STA);
        //WiFi.begin(settings.ssid.c_str(), settings.password.c_str());
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

extern "C" const char *get_var_logicmode() {
    return globalConfig.mode;
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