#include "v1_config.h"
#include <ESPAsyncWebServer.h>
#include "web.h"
#include "wifi.h"

IPAddress local_ip(192, 168, 242, 1);
IPAddress gateway(192, 168, 242, 1);
IPAddress subnet(255, 255, 255, 0);

bool localWifiStarted = false;
bool wifiClientConnected = false;
TaskHandle_t wifiScanTaskHandle = NULL;

void wifiScanTask(void *parameter) {
    wifiScan();
    wifiConnecting = false;
    wifiScanTaskHandle = NULL;
    vTaskDelete(NULL);
}

void onWiFiEvent(WiFiEvent_t event) {
    wifi_mode_t currentMode = WiFi.getMode();
    switch (event) {
        case WIFI_EVENT_WIFI_READY:
            Serial.println("Wifi ready.");
            break;
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
            if (settings.enableWifi && !wifiConnecting && wifiScanTaskHandle == NULL) {
                Serial.println("Attempting reconnect...");
                xTaskCreate(wifiScanTask, "wifiScanTask", 4096, NULL, 1, &wifiScanTaskHandle);
            }
            break;
        case WIFI_EVENT_STA_CONNECTED:
            wifiConnected = true;
            Serial.printf("Station connected to %s.\n", WiFi.SSID().c_str());
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            wifiConnecting = false;
            Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
            if (!webStarted) {
                setupWebServer();
            }
            break;
        case WIFI_EVENT_STA_WPS_ER_TIMEOUT:
            Serial.println("Caught WiFi WPS enrollee timeout.");
            break;
        case WIFI_EVENT_AP_START:
            Serial.printf("Access Point started. IP: %s\n", WiFi.softAPIP().toString().c_str());
            localWifiStarted = true;
            //Serial.printf("DEBUG: localWifiStarted set to TRUE (event)\n");
            if (!webStarted) {
                setupWebServer();
            }
            break;
        case WIFI_EVENT_AP_STOP:
            Serial.println("Access Point stopped.");
            wifiClientConnected = false;
            //Serial.printf("DEBUG: localWifiStarted set to FALSE\n");
            localWifiStarted = false;
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            Serial.println("A device connected to the AP.");
            wifiClientConnected = true;
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            Serial.println("A device disconnected from the AP.");
            wifiClientConnected = false;
            break;
        default:
            Serial.printf("WiFi Event %d on core %d\n", event, xPortGetCoreID());
        break;
    }
}

void startLocalWifi() {
    if (localWifiStarted) {
        Serial.println("Local WiFi already started");
        return;
    }

    Serial.println("Starting AP mode...");
    WiFi.disconnect(true, true);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    WiFi.mode(WIFI_MODE_AP);
    vTaskDelay(pdMS_TO_TICKS(200));

    WiFi.softAPConfig(local_ip, gateway, subnet);
    if (WiFi.softAP("v1display", "password123")) {
        Serial.printf("Fallback AP started. SSID: %s, IP: %s\n", 
            settings.localSSID.c_str(), WiFi.softAPIP().toString().c_str());
        localWifiStarted = true;
        //Serial.printf("DEBUG: localWifiStarted set to TRUE\n");
        if (!webStarted) {
            setupWebServer();
        }
    } else {
        Serial.println("Failed to start fallback Access Point.");
        //Serial.printf("DEBUG: localWifiStarted remains FALSE\n");
    }
}

void wifiSetup() {
    WiFi.mode(static_cast<wifi_mode_t>(settings.wifiMode));
    WiFi.getTxPower();
    WiFi.disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));

    WiFi.onEvent(onWiFiEvent);
    if (settings.wifiMode == WIFI_SETTING_STA) {
        if (wifiScanTaskHandle == NULL) {
            xTaskCreate(wifiScanTask, "wifiScanTask", 4096, NULL, 1, &wifiScanTaskHandle);
        }
    }
    else {
        startLocalWifi();
    }
}

void wifiScan() {
    Serial.println("Scanning for WiFi networks...");
    int networkCount = WiFi.scanNetworks();

    if (networkCount == 0) {
        Serial.println("No networks found");
        if (settings.enableWifi) {
            startLocalWifi();
        }
        WiFi.scanDelete();
        return;
    }

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
    if (!wifiConnected && !localWifiStarted && settings.enableWifi) {
        startLocalWifi();
    }
    WiFi.scanDelete();
}