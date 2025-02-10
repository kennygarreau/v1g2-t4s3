/*
 * Valentine V1 Gen2 Remote Display
 * version: v2.0a
 * Author: Kenny G
 * Date: 2024.December
 * License: GPL 3.0
 */

#include <Arduino.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "BLEDevice.h"
#include "pin_config.h"
#include "v1_config.h"
#include "v1_packet.h"
#include "v1_fs.h"
#include <TinyGPS++.h>
#include "web.h"
#include <ui/ui.h>
#include "lvgl.h"
#include "tft_v2.h"
#include <FreeRTOS.h>

AsyncWebServer server(80);
IPAddress local_ip(192, 168, 242, 1);
IPAddress gateway(192, 168, 242, 1);
IPAddress subnet(255, 255, 255, 0);

BLERemoteService* dataRemoteService;
BLERemoteCharacteristic* infDisplayDataCharacteristic = nullptr;
BLERemoteCharacteristic* clientWriteCharacteristic = nullptr;
BLEClient* pClient = nullptr;
BLEScan* pBLEScan;
bool connected = false;

static bool laserAlert = false;
static String hexData = "";
static String previousHexData = "";
static std::string lastPacket = "";
static std::string bogeyValue, barValue, bandValue, directionValue;

LilyGo_AMOLED amoled;

GPSData gpsData;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
int currentSpeed = 0;
float batteryPercentage = 0.0f;
bool batteryConnected;
bool batteryCharging;
bool isVBusIn;
float voltageInMv = 0.0f;
uint16_t vBusVoltage = 0;

v1Settings settings;
Preferences preferences;

int loopCounter = 0;
unsigned long lastMillis = 0;
static unsigned long lastTick = 0;
const unsigned long uiTickInterval = 5;

const uint8_t notificationOn[] = {0x1, 0x0};

SPIFFSFileManager fileManager;

void requestMute() {
  if (!settings.displayTest) {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqMuteOn(), 7, false);
  }
}

void scanAndConnect() {
  BLEScanResults foundDevices = pBLEScan->start(5);
  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    if (device.haveServiceUUID() && device.isAdvertisingService(BLEUUID(bmeServiceUUID))) {
      Serial.println("V1 found. Attempting to connect...");
      Serial.printf("Device Address: %s\n", device.getAddress().toString().c_str());

      if (pClient->connect(&device)) {
        Serial.println("Connected!");
        pBLEScan->stop();
        return;
      }
      else {
        Serial.println("There was an issue connecting to the V1");
      }
    }
  }
}

void connectToServer() {
  if (!connected) {
    scanAndConnect();
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    connected = true;
  }

  void onDisconnect(BLEClient* pClient) {
    connected = false;
  }
};

// TODO: remove the conversion dependency
static void notifyDisplayCallback(BLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  unsigned long bleCallbackStart = millis();
  hexData = "";
  if (pData) {
    for (size_t i = 0; i < length; i++) {
      char hexBuffer[3];
      sprintf(hexBuffer, "%02X", pData[i]);
      hexData += hexBuffer;
    }
    // uncomment below for debug before entry into the payload calls
    if (hexData != previousHexData) {
      //Serial.print("HEX decode: ");
      //Serial.println(hexData);
      previousHexData = hexData;
    }
  }
  unsigned long bleCallbackLength = millis() - bleCallbackStart;
  // Serial.print("HEX decode time: ");
  // Serial.println(bleCallbackLength);
}


void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi");
      delay(10);
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("WiFi begin succeeded ");
      Serial.println(WiFi.localIP());
      break;
    default:
       // Display additional events???
      break;    
  }
}

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

void scanWiFiNetworks() {
  Serial.println("Scanning for WiFi networks...");
  
  int networkCount = WiFi.scanNetworks();
  
  if (networkCount == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.printf("%d networks found:\n", networkCount);
    for (int i = 0; i < networkCount; ++i) {
      Serial.printf("[%d] SSID: %s, RSSI: %d dBm, Encryption: %s, Channel: %d\n",
        i + 1,
        WiFi.SSID(i).c_str(),
        WiFi.RSSI(i),
        encryptionTypeToString(WiFi.encryptionType(i)),
        WiFi.channel(i));
    }
  }

  WiFi.scanDelete();
}

void wifiSetup() {
  WiFi.onEvent(onWiFiEvent);
}

void wifiConnect() {
 //scanWiFiNetworks();

  WiFi.mode(WIFI_MODE);
  if (WIFI_MODE == WIFI_MODE_AP) {
    WiFi.softAP(settings.ssid, settings.password);
    WiFi.softAPConfig(local_ip, gateway, subnet);
  }
  else {
    WiFi.begin(settings.ssid, settings.password);
  }
}

void queryDeviceInfo(BLEClient* pClient) {
  BLERemoteService* pService = pClient->getService(deviceInfoUUID);

  if (pService == nullptr) {
    Serial.println("Device Information Service not found.");
    return;
  }

  struct CharacteristicMapping {
    const char* name;
    const char* uuid;
    std::string* storage;
  } characteristics[] = {
    {"Manufacturer Name", "2A29", &manufacturerName},
    {"Model Number", "2A24", &modelNumber},
    {"Serial Number", "2A25", &serialNumber},
    {"Hardware Revision", "2A27", &hardwareRevision},
    {"Firmware Revision", "2A26", &firmwareRevision},
    {"Software Revision", "2A28", &softwareRevision}
  };

  for (const auto& charInfo : characteristics) {
    BLERemoteCharacteristic* pCharacteristic = pService->getCharacteristic(BLEUUID(charInfo.uuid));
    if (pCharacteristic != nullptr) {
      *charInfo.storage = pCharacteristic->readValue();
      Serial.printf("%s: %s\n", charInfo.name, charInfo.storage->c_str());
    } else {
      Serial.printf("%s not found.\n", charInfo.name);
    }
  }
}

void displayReader() {
  if (pClient && pClient->isConnected()) {
    set_var_bt_connected(true);
    if (!dataRemoteService) {
      dataRemoteService = pClient->getService(bmeServiceUUID);
      if (dataRemoteService == nullptr) {
        Serial.println("Failed to find infDisplayData service.");
        return;
    }
  }

  std::map<std::string, BLERemoteCharacteristic*>* pCharacteristics = dataRemoteService->getCharacteristics();
  if (pCharacteristics) {
    for (auto& characteristic : *pCharacteristics) {
      if (characteristic.first == clientWriteUUID.toString()) {
        clientWriteCharacteristic = dataRemoteService->getCharacteristic(clientWriteUUID);
        if (clientWriteCharacteristic != nullptr) {
          infDisplayDataCharacteristic = dataRemoteService->getCharacteristic(infDisplayDataUUID);
          infDisplayDataCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
          delay(100);
          
          // If we allow the user to select full blank vs BT illuminated, we may need to modify the buffer size
          if (settings.turnOffDisplay) {
            clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqTurnOffMainDisplay(), 8, false);
            delay(50);
          }

          clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqStartAlertData(), 7, false);
          }
      }
      else if (characteristic.first == infDisplayDataUUID.toString()) {
        infDisplayDataCharacteristic = dataRemoteService->getCharacteristic(infDisplayDataUUID);
        infDisplayDataCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
        }
      }
    }
  }
}

void loadSettings() {
  settings.brightness = preferences.getUInt("brightness", amoled.getBrightness());
  settings.wifiMode = preferences.getString("wifiMode", "WIFI_STA");
  settings.ssid = preferences.getString("ssid", "AdeptaSororitas");
  settings.password = preferences.getString("password", "SaintCelestine");
  settings.disableBLE = preferences.getBool("disableBLE", false);
  settings.displayTest = preferences.getBool("displayTest", false);
  settings.enableGPS = preferences.getBool("enableGPS", true);
  settings.lowSpeedThreshold = preferences.getInt("lowSpeedThres", 35);
  settings.unitSystem = preferences.getString("unitSystem", "Imperial");
  settings.displayOrientation = preferences.getInt("displayOrient", 0);
  settings.timezone = preferences.getString("timezone", "UTC");

  if (settings.displayOrientation == 0 || settings.displayOrientation == 2) {
    settings.isPortraitMode = true;
  } else {
    settings.isPortraitMode = false;
  }

  settings.textColor = preferences.getUInt("textColor", 0xFF0000);
  settings.turnOffDisplay = preferences.getBool("turnOffDisplay", true);
  settings.onlyDisplayBTIcon = preferences.getBool("onlyDispBTIcon", true);

  // selectedConstants = settings.isPortraitMode ? portraitConstants : landscapeConstants;
  // Serial.println("Panel Width: " + String(selectedConstants.MAX_X));
}

void saveSelectedConstants(const DisplayConstants& constants) {
  preferences.putBytes("selConstants", &constants, sizeof(DisplayConstants));
}

void loadSelectedConstants(DisplayConstants& constants) {
  preferences.getBytes("selConstants", &constants, sizeof(DisplayConstants));
}

int getBluetoothSignalStrength() {
  int rssi = pClient->getRssi();
  return rssi;
}

String formatTime(TinyGPSPlus &gps) {
  char timeBuffer[10];
  if (gps.time.isValid()) {
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
  } else {
    snprintf(timeBuffer, sizeof(timeBuffer), "00:00:00");
  }
  return String(timeBuffer);
}

String formatDate(TinyGPSPlus &gps) {
  char dateBuffer[11];
  if (gps.date.isValid()) {
    snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d", gps.date.month(), gps.date.day(), gps.date.year());
  } else {
    snprintf(dateBuffer, sizeof(dateBuffer), "00/00/0000");
  }
  return String(dateBuffer);
}

void setup()
{
  Serial.begin();
  analogReadResolution(12);
  delay(1000);

  Serial.println("Reading initial settings...");
  preferences.begin("settings", false);
  loadSettings();

  bool rslt = false;
  rslt = amoled.begin();
  amoled.setBrightness(settings.brightness);
  Serial.println("============================================");
  Serial.print("    Board Name:LilyGo AMOLED "); Serial.println(amoled.getName());
  Serial.println("============================================");
  
  if (!rslt) {
    while (1) {
      Serial.println("The board model cannot be detected, please raise the Core Debug Level to ERROR");
      delay(1000);
    }
  }

  beginLvglHelper(amoled);
  //loadSelectedConstants(selectedConstants);
  // if not initialized yet (first boot) - initialize settings
  // if (selectedConstants.MAX_X == 0 && selectedConstants.MAX_Y == 0) {
  //   selectedConstants = settings.isPortraitMode ? portraitConstants : landscapeConstants;

  //   Serial.println("CONFIG: this appears to be the first boot, saving defaults");
  //   saveSelectedConstants(selectedConstants);
  // }

  preferences.end();

  if (settings.enableGPS) {
    Serial.println("Initializing GPS...");
    gpsSerial.begin(BAUD_RATE, SERIAL_8N1, RXD, TXD);
  }
 ui_init();

  // TODO: add a failsafe handler for invalid wifi credentials
  wifiSetup();
  wifiConnect();

  if (!fileManager.init()) {
    Serial.println("Failed to initialize SPIFFS");
    return;
  }

 setupWebServer();

 if (!settings.disableBLE && !settings.displayTest) {
  Serial.println("Searching for V1 bluetooth..");
  BLEDevice::init("ESP32 Client");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);

  connectToServer();

  if (connected) {
    displayReader();
    infDisplayDataCharacteristic->registerForNotify(notifyDisplayCallback);
  } else {
    Serial.println("Could not find a V1 connection");
  }

  queryDeviceInfo(pClient);
 }
 Serial.println("v1g2 firmware version: " + String(FIRMWARE_VERSION));
}

void loop() {  
  static bool configHasRun = false;
  static unsigned long lastGPSUpdate = 0;
  unsigned long gpsMillis = millis();

  if (settings.disableBLE == false && !settings.displayTest) {
    if (pClient->isConnected()) {
      connected = true;
    } else {
      if (!settings.displayTest) {
        Serial.println("disconnected - attempting reconnect");
        set_var_bt_connected(false);
        pBLEScan = BLEDevice::getScan();
        pBLEScan->setActiveScan(true);
        connectToServer();
        
        if (connected) {
          displayReader();
          infDisplayDataCharacteristic->registerForNotify(notifyDisplayCallback);
        } else {
        Serial.println("Could not find a V1 connection");
        }
      }
    }
 }
  if (settings.displayTest) {
    // Serial.println("========================");
    // Serial.println("====  DISPLAY TEST  ====");
    // Serial.println("========================");

    std::string packets[] = {"AAD6EA430713291D21858800E8AB", "AAD8EA31095B1F38280C0000E7AB", "AAD6EA4307235E569283240000AB", "AAD6EA430733878CB681228030AB"};
    //std::string packets[] = {"AAD8EA31095B1F38280C0000E7AB"};

    for (const std::string& packet : packets) {
      //unsigned long decodeLoopStart = millis();
      PacketDecoder decoder(packet); 
      decoder.decode(settings.lowSpeedThreshold, currentSpeed);
      // unsigned long decodeLoopEnd = millis() - decodeLoopStart;
      // Serial.print("Decode loop: ");
      // Serial.println(decodeLoopEnd);
      delay(50);
    }
  }

  // decode loop takes ~2ms
  std::string packet = hexData.c_str();
  if (packet != lastPacket) {
    //unsigned long decodeLoopStart = millis();
    PacketDecoder decoder(packet);
    std::string decoded = decoder.decode(settings.lowSpeedThreshold, currentSpeed);
    lastPacket = packet;

    // unsigned long decodeLoopEnd = millis() - decodeLoopStart;
    // Serial.print("Decode loop: ");
    // Serial.println(decodeLoopEnd);
  } 
  
  unsigned long currentMillis = millis();

  if (currentMillis - lastMillis >= 2000) {
    //Serial.println("Loops executed: " + String(loopCounter)); // uncomment for loop profiling
    ui_tick_statusBar();

    isVBusIn = amoled.isVbusIn();
    if (isVBusIn) {
      vBusVoltage = amoled.getVbusVoltage();
    }

    batteryCharging = amoled.isCharging();
    uint16_t batteryVoltage = amoled.getBattVoltage();
    voltageInMv = batteryVoltage; // cast this to float
    batteryPercentage = ((voltageInMv - EMPTY_VOLTAGE) / (FULLY_CHARGED_VOLTAGE - EMPTY_VOLTAGE)) * 100.0;
    if (batteryPercentage > 100) batteryPercentage = 100;
    if (batteryPercentage < 0) batteryPercentage = 0;

    // This will obtain the User-defined settings (eg. disabling certain bands)
    if (!configHasRun && !settings.displayTest) {
      if (globalConfig.muteTo.empty()) {
        Serial.print("Awaiting user settings...");
        clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqUserBytes(), 7, false);
      } else {
        Serial.println("User settings obtained!");
        configHasRun = true;
        set_var_prio_alert_freq("");
      }
    }
    uint32_t cpuIdle = lv_timer_get_idle();  
    gpsData.cpuBusy = 100 - cpuIdle;  
    //Serial.printf("CPU Busy: %u%%\n", gpsData.cpuBusy);

    lastMillis = currentMillis;
    loopCounter = 0;
    checkReboot();

  }
  
  if (settings.enableGPS && (currentMillis - lastGPSUpdate >= 250)) {
    lastGPSUpdate = currentMillis; 

    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        //Serial.print(c);
        gps.encode(c);
      }
      if (gps.location.isUpdated()) {
        if (gps.location.isValid()) {
          gpsData.latitude = gps.location.lat();
          gpsData.longitude = gps.location.lng();
          gpsData.satelliteCount = gps.satellites.value();
          gpsData.course = gps.course.deg();
          gpsData.date = formatDate(gps);
          gpsData.time = formatTime(gps);
          gpsData.hdop = static_cast<double>(gps.hdop.value()) / 100.0;

          if (gpsData.hdop < 2) {
            gpsData.signalQuality = "excellent";
          } 
          else if (gpsData.hdop <= 5) {
            gpsData.signalQuality = "good";
          } 
          else if (gpsData.hdop <= 10) {
            gpsData.signalQuality = "moderate";
          } 
          else {
            gpsData.signalQuality = "poor";
          }

          if (settings.unitSystem == "Metric") {
            gpsData.speed = gps.speed.kmph();
            gpsData.altitude = gps.altitude.meters();
            currentSpeed = static_cast<int>(gps.speed.kmph());
          } else {
            gpsData.speed = gps.speed.mph();
            gpsData.altitude = gps.altitude.feet();
            currentSpeed = static_cast<int>(gps.speed.mph());
          }
        }
      }
  }
  
  // TODO: add deep sleep for BLE disconnect timeout
  unsigned long now = millis();
  if (now - lastTick >= uiTickInterval) {
    lastTick = now;
    ui_tick();
    unsigned long startTime = millis();
    lv_task_handler();
    // unsigned long endTime = millis();
    // if (endTime - startTime > 1) {
    //   Serial.printf("lv_task_handler execution time: %lu ms\n", endTime - startTime);
    // }
  }

  // 500us delay: ~1500 loops/s
  // 1000us delay: ~850 loops/s
  loopCounter++;
  delayMicroseconds(10);
  yield();
}