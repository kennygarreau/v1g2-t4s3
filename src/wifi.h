#ifndef WIFI_H
#define WIFI_H

#include <ESPAsyncWebServer.h>
#include <WiFi.h>

extern TaskHandle_t wifiScanTaskHandle;

void onWiFiEvent(WiFiEvent_t event);
void wifiSetup();
void wifiScan();
void wifiScanTask(void* parameter);
void stopWifi();

#endif