#ifndef WIFI_H
#define WIFI_H

#include <ESPAsyncWebServer.h>

void onWiFiEvent(WiFiEvent_t event);
void wifiSetup();
void wifiConnect();
void startWifiAsync();
void handleWifi();

#endif