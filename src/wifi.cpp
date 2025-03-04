#include <v1_config.h>
#include <ESPAsyncWebServer.h>
#include "wifi.h"

IPAddress local_ip(192, 168, 242, 1);
IPAddress gateway(192, 168, 242, 1);
IPAddress subnet(255, 255, 255, 0);

const char* encryptionTypeToString(wifi_auth_mode_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA/PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2/PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2/PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2/Enterprise";
        default: return "Unknown";
    }
}

void onWiFiEvent(WiFiEvent_t event) {
    wifi_mode_t currentMode = WiFi.getMode();
    Serial.printf("WiFi Event %d on core %d\n", event, xPortGetCoreID());

    if (currentMode == WIFI_MODE_STA) {
        switch (event) {
            case SYSTEM_EVENT_STA_DISCONNECTED:
                wifiConnected = false;
                Serial.println("WiFi disconnected. Attempting reconnect...");

                if (!wifiConnecting) {
                    //wifiConnecting = true;
                    //WiFi.disconnect();
                    //WiFi.reconnect();
                    wifiScan();
                }
                break;
            case SYSTEM_EVENT_STA_GOT_IP:
                wifiConnected = true;
                wifiConnecting = false;
                Serial.printf("Connected to %s! IP Address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
                break;
            default:
                //Serial.printf("Unhandled WiFi Event (STA): %d\n", event);
                break;
        }
    } else if (currentMode == WIFI_MODE_AP) {
        switch (event) {
            case SYSTEM_EVENT_AP_START:
                Serial.printf("Access Point started. IP: %s\n", WiFi.softAPIP().toString().c_str());
                break;
            case SYSTEM_EVENT_AP_STOP:
                Serial.println("Access Point stopped.");
                break;
            case SYSTEM_EVENT_AP_STACONNECTED:
                Serial.println("A device connected to the AP.");
                break;
            case SYSTEM_EVENT_AP_STADISCONNECTED:
                Serial.println("A device disconnected from the AP.");
                break;
            default:
                //Serial.printf("Unhandled WiFi Event (AP): %d\n", event);
                break;
        }
    }
}

void wifiSetup() {
    WiFi.onEvent(onWiFiEvent);
}

void startWifiAsync() {
    WiFi.mode(static_cast<wifi_mode_t>(settings.wifiMode));
    //Serial.printf("Wifi Mode set to: %d\n", settings.wifiMode);
    
    if (WiFi.getMode() == WIFI_MODE_AP) {
        Serial.println("Starting Access Point...");
        WiFi.softAPConfig(local_ip, gateway, subnet);
        if (WiFi.softAP(settings.localSSID.c_str(), settings.localPW.c_str(), 9)) {
            Serial.printf("AP started. SSID: %s, IP: %s, Channel: 9\n", 
                settings.localSSID.c_str(), WiFi.softAPIP().toString().c_str());
        } else {
            Serial.println("Failed to start Access Point.");
        }
    } else {
        Serial.println("Attempting to connect to WiFi...");
        wifiConnecting = true;
        wifiStartTime = millis();
        for (const auto& cred : settings.wifiCredentials) {
            Serial.printf("Trying to connect to: %s", cred.ssid.c_str());
        
            WiFi.disconnect(true);
            WiFi.mode(WIFI_STA);
            WiFi.begin(cred.ssid.c_str(), cred.password.c_str());
        
            unsigned long startAttemptTime = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 7500) {
                delay(100);
                Serial.print(".");
            }
        
            if (WiFi.status() == WL_CONNECTED) {
                //Serial.printf("\nConnected to WiFi: %s, IP: %s\n", 
                //    cred.ssid.c_str(), WiFi.localIP().toString().c_str());
                return;
            }
        
            Serial.println("\nFailed to connect. Trying next network...");
        }
    }
}

void handleWifi() {
    if (wifiConnecting) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("\nConnected to WiFi: %s, IP: %s\n", 
                WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
            wifiConnecting = false;
        } else if (millis() - wifiStartTime > 15000) {
            Serial.println("WiFi connection timeout. Starting AP mode...");
            WiFi.mode(WIFI_MODE_AP);
            WiFi.softAPConfig(local_ip, gateway, subnet);
            if (WiFi.softAP(settings.localSSID.c_str(), settings.localPW.c_str(), 9)) {
                Serial.printf("Fallback AP started. SSID: %s, IP: %s, Channel: 9\n", 
                    settings.localSSID.c_str(), WiFi.softAPIP().toString().c_str());
            } else {
                Serial.println("Failed to start fallback Access Point.");
            }
            wifiConnecting = false;
        }
    }
}

void wifiScan() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Scanning for WiFi networks...");
    int networkCount = WiFi.scanNetworks();
    if (networkCount == 0) {
        Serial.println("No networks found");
    } else {
        Serial.printf("%d networks found:\n", networkCount);
        for (int i = 0; i < networkCount; i++) {
            Serial.printf("%d: %s (RSSI: %d) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), 
                            WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secured");
        }
        for (auto &cred : settings.wifiCredentials) {
            for (int i = 0; i < networkCount; i++) {
                if (WiFi.SSID(i) == cred.ssid) {
                    wifiConnecting = true;
                    Serial.printf("Attempting to connect to: %s", cred.ssid.c_str());
                    WiFi.begin(cred.ssid.c_str(), cred.password.c_str());

                    unsigned long startAttemptTime = millis();
                    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
                        Serial.print(".");
                        delay(500);
                    }
                    Serial.println();

                    if (WiFi.status() == WL_CONNECTED) {
                        return;
                    } else {
                        Serial.printf("Failed to connect to %s\n", cred.ssid.c_str());
                    }
                }
            }
        }
        Serial.println("No known networks available.");
        wifiConnecting = false;
    }
    WiFi.scanDelete();
}