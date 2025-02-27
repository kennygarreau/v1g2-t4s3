/*
 * Valentine V1 Gen2 Remote Display
 * version: v1.0
 * Author: Kenny G
 * Date: February 2025
 * License: MIT
 */

#include <Arduino.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "BLEDevice.h"
#include "v1_config.h"
#include "v1_packet.h"
#include "v1_fs.h"
#include <TinyGPS++.h>
#include "web.h"
#include "wifi.h"
#include <ui/ui.h>
#include "lvgl.h"
#include "tft_v2.h"
#include <FreeRTOS.h>
#include <ezTime.h>
#include "gps.h"

AsyncWebServer server(80);

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
bool batteryConnected, batteryCharging, isVBusIn, wifiConnecting;
float voltageInMv = 0.0f;
uint16_t vBusVoltage = 0;
unsigned long wifiStartTime = 0;
bool gpsAvailable = false;
bool wifiConnected = false;
unsigned long lastValidGPSUpdate = 0;
unsigned long lastBLEAttempt = 0;

v1Settings settings;
Preferences preferences;
Timezone tz;

int loopCounter = 0;
unsigned long lastMillis = 0;
const unsigned long uiTickInterval = 5;

const uint8_t notificationOn[] = {0x1, 0x0};

SPIFFSFileManager fileManager;

void requestMute() {
  if (!settings.displayTest) {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqMuteOn(), 7, false);
  }
}

void scanAndConnect() {
  BLEScanResults foundDevices = pBLEScan->start(5, false);
  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    if (device.haveServiceUUID() && device.isAdvertisingService(BLEUUID(bmeServiceUUID))) {
      Serial.printf("V1 found! Attempting to connect to: %s\n", device.getAddress().toString().c_str());

      if (pClient->connect(&device)) {
        Serial.println("Connected!");
        connected = true;
        pBLEScan->clearResults();
        return;
      }
      else {
        Serial.println("There was an issue connecting to the V1");
      }
    }
  }
  pBLEScan->clearResults();
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
    if (hexData != previousHexData) {
      // uncomment below for debug before entry into the payload calls
      //Serial.printf("HEX decode: %s", hexData.c_str());
      previousHexData = hexData;
    }
  }
  unsigned long bleCallbackLength = millis() - bleCallbackStart;
  //Serial.printf("HEX decode time: %d", bleCallbackLength);
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

  int mode = preferences.getInt("wifiMode", WIFI_SETTING_STA);
  if (mode < WIFI_SETTING_AP || mode > WIFI_SETTING_APSTA) {
      mode = WIFI_SETTING_STA;
  }
  settings.wifiMode = static_cast<WiFiModeSetting>(mode);

  settings.localSSID = preferences.getString("localSSID", "v1display");
  settings.localPW = preferences.getString("localPW", "password123");
  settings.disableBLE = preferences.getBool("disableBLE", false);
  settings.displayTest = preferences.getBool("displayTest", false);
  settings.enableGPS = preferences.getBool("enableGPS", true);
  settings.enableWifi = preferences.getBool("enableWifi", true);
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

  //loadSelectedConstants(selectedConstants);
  // if not initialized yet (first boot) - initialize settings
  // if (selectedConstants.MAX_X == 0 && selectedConstants.MAX_Y == 0) {
  //   selectedConstants = settings.isPortraitMode ? portraitConstants : landscapeConstants;

  //   Serial.println("CONFIG: this appears to be the first boot, saving defaults");
  //   saveSelectedConstants(selectedConstants);
  // }
  // selectedConstants = settings.isPortraitMode ? portraitConstants : landscapeConstants;

  settings.wifiCredentials.clear();
  int networkCount = preferences.getInt("networkCount", 0);

  for (int i = 0; i < networkCount; i++) {
      String ssid = preferences.getString(("wifi_ssid_" + String(i)).c_str(), "");
      String password = preferences.getString(("wifi_pass_" + String(i)).c_str(), "");

      if (ssid.length() > 0) {
        Serial.printf("Saved network %d: %s\n", i, ssid.c_str());
        settings.wifiCredentials.push_back({ssid, password});
      }
  }
}

void saveSelectedConstants(const DisplayConstants& constants) {
  preferences.putBytes("selConstants", &constants, sizeof(DisplayConstants));
}

void loadSelectedConstants(DisplayConstants& constants) {
  preferences.getBytes("selConstants", &constants, sizeof(DisplayConstants));
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
  preferences.end();

  if (settings.enableGPS) {
    Serial.println("Initializing GPS...");
    gpsSerial.begin(BAUD_RATE, SERIAL_8N1, RXD, TXD);
  }

  if (!fileManager.init()) {
    Serial.println("Failed to initialize SPIFFS");
    return;
  }

  ui_init();
  ui_tick();
  lv_task_handler();

 if (!settings.disableBLE && !settings.displayTest) {
  Serial.println("Searching for V1 bluetooth..");
  BLEDevice::init("ESP32 Client");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  /*
  scanAndConnect();

  if (connected) {
    displayReader();
    infDisplayDataCharacteristic->registerForNotify(notifyDisplayCallback);
  } else {
    Serial.println("Could not find a V1 connection");
  }
  queryDeviceInfo(pClient);
  */
  }

  Serial.println("v1g2 firmware version: " + String(FIRMWARE_VERSION));
  
  wifiSetup();
  wifiScan();
  startWifiAsync();
  setupWebServer();

  tz.setLocation(settings.timezone);

}

void loop() {  
  static bool configHasRun = false;
  static unsigned long lastGPSUpdate = 0;
  static unsigned long lastWifiReconnect = 0;
  static unsigned long lastTick = 0;
  unsigned long gpsMillis = millis();

  handleWifi();

  if (WiFi.getMode() == WIFI_MODE_STA && WiFi.status() != WL_CONNECTED && !wifiConnecting) {
    if (millis() - lastWifiReconnect > 10000) {
      Serial.println("WiFi lost. Reconnecting...");
      WiFi.disconnect();
      WiFi.reconnect();
      lastWifiReconnect = millis();
    }
  }

  if (!settings.disableBLE && !settings.displayTest) {
    if (!pClient->isConnected()) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastBLEAttempt >= BLE_RETRY_INTERVAL) {
        lastBLEAttempt = currentMillis;
        Serial.println("Disconnected - attempting reconnect...");
        set_var_bt_connected(false);
        scanAndConnect();

      if (connected) {
        displayReader();
        infDisplayDataCharacteristic->registerForNotify(notifyDisplayCallback);
        queryDeviceInfo(pClient);
      } else {
        Serial.println("Could not find a V1 connection");
      }
    }
  }
}

  if (settings.displayTest) {

    std::string packets[] = {"AAD6EA430713291D21858800E8AB", "AAD8EA31095B1F38280C0000E7AB", "AAD6EA4307235E569283240000AB", "AAD6EA430733878CB681228030AB"};
    //std::string packets[] = {"AAD8EA31095B5B0724248CCC5457AB", "AAD8EA310906060F24248CCC54B5AB"};

    for (const std::string& packet : packets) {
      //unsigned long decodeLoopStart = millis();
      PacketDecoder decoder(packet); 
      decoder.decode(settings.lowSpeedThreshold, currentSpeed);
      //Serial.printf("Decode loop: %lu\n", millis() - decodeLoopStart);
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

    //Serial.printf("Decode loop: %lu\n", millis() - decodeLoopStart);
  } 
  
  unsigned long currentMillis = millis();

  if (currentMillis - lastMillis >= 2000) {
    //Serial.println("Loops executed: " + String(loopCounter)); // uncomment for loop profiling
    ui_tick_statusBar();
    gpsData.btStr = getBluetoothSignalStrength();

    if (WiFi.getMode() == WIFI_MODE_AP && WiFi.softAPgetStationNum() == 0) {
      gpsData.connectedClients = 0;
    } else {
      gpsData.connectedClients = WiFi.softAPgetStationNum();
    }

    isVBusIn = amoled.isVbusIn();
    if (isVBusIn) {
      vBusVoltage = amoled.getVbusVoltage();
    }

    batteryCharging = amoled.isCharging();
    uint16_t batteryVoltage = amoled.getBattVoltage();
    voltageInMv = batteryVoltage; // cast this to float
    batteryPercentage = ((voltageInMv - EMPTY_VOLTAGE) / (FULLY_CHARGED_VOLTAGE - EMPTY_VOLTAGE)) * 100.0;
    batteryPercentage = constrain(batteryPercentage, 0, 100);

    // This will obtain the User-defined settings (eg. disabling certain bands)
    if (!configHasRun && !settings.displayTest) {
      if (globalConfig.muteTo.empty()) {
        gpsData.totalHeap = ESP.getHeapSize();
        gpsData.totalPsram = ESP.getPsramSize();
        if (connected) {
          Serial.print("Awaiting user settings...");
          clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqUserBytes(), 7, false); 
        }
      } else {
        Serial.println("User settings obtained!");
        configHasRun = true;
        set_var_prio_alert_freq("");
      }
    }
    uint32_t cpuIdle = lv_timer_get_idle();  
    gpsData.cpuBusy = 100 - cpuIdle;
    gpsData.freeHeap = ESP.getFreeHeap();
    gpsData.freePsram = ESP.getFreePsram();
    
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
    if (gps.location.isUpdated() && gps.location.isValid()) {
      gpsData.latitude = gps.location.lat();
      gpsData.longitude = gps.location.lng();
      gpsData.satelliteCount = gps.satellites.value();
      gpsData.course = gps.course.deg();
      // gpsData.date = formatDate(gps);
      // gpsData.time = formatTime(gps);
      gpsData.date = formatLocalDate(gps);
      gpsData.time = formatLocalTime(gps);
      gpsData.hdop = static_cast<double>(gps.hdop.value()) / 100.0;

      gpsData.signalQuality = (gpsData.hdop < 2) ? "excellent" : (gpsData.hdop <= 5) ? "good" : (gpsData.hdop <= 10) ? "moderate" : "poor";

      if (settings.unitSystem == "Metric") {
        gpsData.speed = gps.speed.kmph();
        gpsData.altitude = gps.altitude.meters();
        currentSpeed = static_cast<int>(gps.speed.kmph());
      } else {
        gpsData.speed = gps.speed.mph();
        gpsData.altitude = gps.altitude.feet();
        currentSpeed = static_cast<int>(gps.speed.mph());
      }
      gpsAvailable = true;
      lastValidGPSUpdate = currentMillis;
    }
    if (currentMillis - lastValidGPSUpdate > 5000) {
      gpsAvailable = false;
    }
  }
  
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

  loopCounter++;
  delayMicroseconds(10);
  yield();
}
