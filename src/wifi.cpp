#include "v1_config.h"
#include <ESPAsyncWebServer.h>
#include "wifi.h"

IPAddress local_ip(192, 168, 242, 1);
IPAddress gateway(192, 168, 242, 1);
IPAddress subnet(255, 255, 255, 0);

bool localWifiStarted = false;

void wifiScanTask(void *parameter) {
    wifiScan();
    vTaskDelete(NULL);
}

/*
void reconnectTask(void *param) {
    Serial.println("Reconnecting to WiFi...");
    xTaskCreate(wifiScanTask, "wifiScanTask", 4096, NULL, 1, NULL);
    vTaskDelete(NULL);
}
*/

void onWiFiEvent(WiFiEvent_t event) {
    wifi_mode_t currentMode = WiFi.getMode();

    if (currentMode == WIFI_MODE_STA) {
        switch (event) {
            case WIFI_EVENT_STA_START:
                Serial.println("WiFi starting...");
                break;
            case WIFI_EVENT_STA_STOP:
                Serial.println("WiFi stopping...");
                wifiConnected = false;
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                wifiConnected = false;
                Serial.println("WiFi disconnected. Attempting reconnect...");
                if (!wifiConnecting) {
                    xTaskCreate(wifiScanTask, "wifiScanTask", 8192, NULL, 1, NULL);
                }
                break;
            case SYSTEM_EVENT_STA_GOT_IP:
                wifiConnected = true;
                wifiConnecting = false;
                Serial.printf("Connected to %s! IP Address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
                break;
            default:
                Serial.printf("WiFi Event %d on core %d\n", event, xPortGetCoreID());
                break;
        }
    } else if (currentMode == WIFI_MODE_AP) {
        switch (event) {
            case WIFI_EVENT_AP_START:
                Serial.printf("Access Point started. IP: %s\n", WiFi.softAPIP().toString().c_str());
                break;
            case WIFI_EVENT_AP_STOP:
                Serial.println("Access Point stopped.");
                break;
            case SYSTEM_EVENT_AP_STACONNECTED:
                Serial.println("A device connected to the AP.");
                break;
            case SYSTEM_EVENT_AP_STADISCONNECTED:
                Serial.println("A device disconnected from the AP.");
                break;
            default:
                Serial.printf("WiFi Event %d on core %d\n", event, xPortGetCoreID());
            break;
        }
    }
    else if (currentMode == WIFI_MODE_NULL) {
        wifiConnected = false;
        localWifiStarted = false;
    }
}

 void startLocalWifi() {
    Serial.println("Starting AP mode...");
    WiFi.disconnect();

    WiFi.mode(WIFI_MODE_AP);
    delay(100);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    if (WiFi.softAP("v1display", "password123")) {
        Serial.printf("Fallback AP started. SSID: %s, IP: %s\n", 
            settings.localSSID.c_str(), WiFi.softAPIP().toString().c_str());
        localWifiStarted = true;
    } else {
        Serial.println("Failed to start fallback Access Point.");
    }
}

void wifiSetup() {
    WiFi.mode(static_cast<wifi_mode_t>(settings.wifiMode));
    WiFi.disconnect();
    WiFi.onEvent(onWiFiEvent);
    if (settings.wifiMode == WIFI_SETTING_STA) {
        xTaskCreate(wifiScanTask, "wifiScanTask", 8192, NULL, 1, NULL);
    } else 
    {
        startLocalWifi();
    }
}

void wifiScan() {
    Serial.println("Scanning for WiFi networks...");
    int networkCount = WiFi.scanNetworks();
    if (networkCount == 0) {
        Serial.println("No networks found");
        startLocalWifi();
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
                    Serial.printf("Attempting to connect to: %s\n", cred.ssid.c_str());
                    WiFi.begin(cred.ssid.c_str(), cred.password.c_str());

                    unsigned long startAttemptTime = millis();
                    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
                        Serial.print(".");
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    Serial.println();

                    if (WiFi.status() == WL_CONNECTED) {
                        return;
                    } else {
                        Serial.printf("Failed to connect to %s\n", cred.ssid.c_str());
                    }
                    vTaskDelay(pdMS_TO_TICKS(100)); 
                }
            }
        }
        Serial.println("No known networks available.");
        if (!wifiConnected && !localWifiStarted) {
            startLocalWifi();
        }
    }
    WiFi.scanDelete();
    vTaskDelay(pdMS_TO_TICKS(100)); 
}