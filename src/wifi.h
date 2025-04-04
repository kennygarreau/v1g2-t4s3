#ifndef WIFI_H
#define WIFI_H

#include <ESPAsyncWebServer.h>
//#include <WiFi.h>

enum WiFiModeSetting {
    WIFI_SETTING_STA = 1,
    WIFI_SETTING_AP = 2,  
    WIFI_SETTING_APSTA = 3
};

void onWiFiEvent(WiFiEvent_t event);
void wifiSetup();
void wifiScan();
void wifiScanTask(void* parameter);

#endif