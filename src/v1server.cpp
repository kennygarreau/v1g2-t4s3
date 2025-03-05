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
#include <NimBLEDevice.h>
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

NimBLERemoteService* dataRemoteService = nullptr;
NimBLERemoteCharacteristic* infDisplayDataCharacteristic = nullptr;
NimBLERemoteCharacteristic* clientWriteCharacteristic = nullptr;
NimBLEClient* pClient = nullptr;
NimBLEScan* pBLEScan = nullptr;
static constexpr uint32_t scanTimeMs = 5 * 1000;

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
bool bleInit = true;

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

// TODO: remove the conversion dependency
static void notifyDisplayCallback(NimBLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
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

void queryDeviceInfo(NimBLEClient* pClient) {
  NimBLERemoteService* pService = pClient->getService(deviceInfoUUID);

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
    NimBLERemoteCharacteristic* pCharacteristic = pService->getCharacteristic(NimBLEUUID(charInfo.uuid));
    if (pCharacteristic != nullptr) {
      *charInfo.storage = pCharacteristic->readValue();

      size_t pos = charInfo.storage->find('\0');
      if (pos != std::string::npos) {
        charInfo.storage->erase(pos);
      }

      Serial.printf("%s: %s\n", charInfo.name, charInfo.storage->c_str());
    } else {
      Serial.printf("%s not found.\n", charInfo.name);
    }
  }
}

void displayReader(NimBLEClient* pClient) {

  dataRemoteService = pClient->getService(bmeServiceUUID);
  if (dataRemoteService) {
      infDisplayDataCharacteristic = dataRemoteService->getCharacteristic(infDisplayDataUUID);
      if (infDisplayDataCharacteristic) {
          if (infDisplayDataCharacteristic->canNotify()) {
              infDisplayDataCharacteristic->subscribe(true, notifyDisplayCallback);
              Serial.println("Subscribed to alert notifications.");
          } else {
              Serial.println("Characteristic does not support notifications.");
          }
      } else {
          Serial.println("Failed to find infDisplayDataCharacteristic.");
      }
  } else {
      Serial.println("Failed to find bmeServiceUUID.");
  }

  infDisplayDataCharacteristic = dataRemoteService->getCharacteristic(infDisplayDataUUID);
  clientWriteCharacteristic = dataRemoteService->getCharacteristic(clientWriteUUID);
  if (infDisplayDataCharacteristic && clientWriteCharacteristic) {
    NimBLERemoteDescriptor* pDesc = infDisplayDataCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
    if (pDesc) {
      pDesc->writeValue((uint8_t*)notificationOn, 2, true);
    }
    delay(50);
    if (settings.turnOffDisplay) {
      uint8_t value = settings.onlyDisplayBTIcon ? 0x01 : 0x00;
      clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqTurnOffMainDisplay(value), 8, false);
      delay(50);
    }
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqStartAlertData(), 7, false);
  }
}

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    Serial.printf("BLE Connected to: %s\n", pClient->getPeerAddress().toString().c_str());
    Serial.printf("BLE Client Connected on core %d\n", xPortGetCoreID());
    bt_connected = true;
    bleInit = true;
    std::vector<NimBLERemoteService*> services = pClient->getServices();
    
  }
  void onDisconnect(NimBLEClient* pClient, int reason) override {
    Serial.printf("%s Disconnected, reason = %d - Restarting scan in 2s\n", 
                  pClient->getPeerAddress().toString().c_str(), reason);
    Serial.printf("BLE Client disconnected on core %d\n", xPortGetCoreID());

    bt_connected = false;
    delay(2000);
    NimBLEDevice::getScan()->start(scanTimeMs);
  }
} clientCallbacks;


class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(bmeServiceUUID)) {
      NimBLEDevice::getScan()->stop();

      std::string deviceAddr = advertisedDevice->getAddress().toString();
      if (deviceAddr.empty()) {
        Serial.println("Invalid device address.");
        return;
      }
      Serial.printf("V1 found! Attempting to connect to: %s\n", deviceAddr.c_str());

      if (!pClient) {
        Serial.println("No disconnected client available, creating new one...");
        pClient = NimBLEDevice::createClient(advertisedDevice->getAddress());
        if (!pClient) {
          Serial.printf("Failed to create client\n");
          return;
        }
      }

      if (!pClient->connect(true, true, false)) {
        Serial.println("Failed to connect, deleting client...");
        NimBLEDevice::deleteClient(pClient);
        return;
      }

      pClient->setClientCallbacks(&clientCallbacks, false);
    }
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override {
    //Serial.println("Scan ended, restarting scan...");
    NimBLEDevice::getScan()->start(scanTimeMs);
  }
} scanCallbacks;


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

void setup()
{
  Serial.begin();
  analogReadResolution(12);
  delay(500);

  Serial.println("Reading initial settings...");
  preferences.begin("settings", false);
  loadSettings();

  bool rslt = amoled.begin();
  if (rslt) {
    amoled.setBrightness(settings.brightness);
    Serial.println("============================================");
    Serial.print("    Board Name:LilyGo AMOLED "); Serial.println(amoled.getName());
    Serial.println("============================================");
  }
  else {
    while (1) {
      Serial.println("The board model cannot be detected, please raise the Core Debug Level to ERROR");
      delay(1000);
    }
  }

  beginLvglHelper(amoled);
  preferences.end();

  Serial.printf("Setup running on core %d\n", xPortGetCoreID());

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
    NimBLEDevice::init("Async Client");
    NimBLEDevice::setPower(3);
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(&scanCallbacks);
    pScan->setInterval(45);
    pScan->setWindow(45);
    pScan->setActiveScan(true);
    pScan->start(scanTimeMs);
  }

  wifiSetup();
  wifiScan();
  setupWebServer();

  Serial.println("v1g2 firmware version: " + String(FIRMWARE_VERSION));
}

void loop() {  
  static bool configHasRun = false;
  static unsigned long lastGPSUpdate = 0;
  static unsigned long lastWifiReconnect = 0;
  static unsigned long lastTick = 0;
  unsigned long gpsMillis = millis();

  if (bt_connected && bleInit) {
    displayReader(pClient);
    bleInit = false;
  }

  // in case we get disconnected for a while
  if (WiFi.getMode() == WIFI_MODE_STA && !wifiConnected) {
    if (millis() - lastWifiReconnect > 30000) {
      Serial.println("WiFi lost. Reconnecting...");
      wifiScan();
      lastWifiReconnect = millis();
    }
  }

  if (settings.displayTest) {

    // TODO: generate more synthetic packets as 31 paints the arrow/bars/band but the alert table doesn't match 1:1
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

    if (WiFi.getMode() == WIFI_MODE_AP && WiFi.softAPgetStationNum() == 0) {
      gpsData.connectedClients = 0;
    } else {
      gpsData.connectedClients = WiFi.softAPgetStationNum();
    }

    isVBusIn = amoled.isVbusIn();
    if (isVBusIn) {
      vBusVoltage = amoled.getVbusVoltage();
    }
    if (bt_connected) {
      gpsData.btStr = getBluetoothSignalStrength();
      clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqBatteryVoltage(), 7, false); 
      clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqCurrentVolume(), 7, false);
    }
    
    batteryCharging = amoled.isCharging();
    uint16_t espVoltage = amoled.getBattVoltage();
    voltageInMv = espVoltage; // cast this to float
    batteryPercentage = ((voltageInMv - EMPTY_VOLTAGE) / (FULLY_CHARGED_VOLTAGE - EMPTY_VOLTAGE)) * 100.0;
    batteryPercentage = constrain(batteryPercentage, 0, 100);

    // This will obtain the User-defined settings (eg. disabling certain bands)
    if (!configHasRun && !settings.displayTest) {
      if (globalConfig.muteTo.empty()) {
        gpsData.totalHeap = ESP.getHeapSize();
        gpsData.totalPsram = ESP.getPsramSize();
        gpsData.totalStorageKB = fileManager.getStorageTotal();
        gpsData.usedStorageKB = fileManager.getStorageUsed();

        if (bt_connected) {
          Serial.print("Awaiting user settings...");
          clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqCurrentVolume(), 7, false);
          delay(20);
          clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqUserBytes(), 7, false);
        }
      } else {
        Serial.println("User settings obtained!");

        lv_obj_t * scr = lv_scr_act();
        if (scr) {
          lv_obj_add_event_cb(scr, main_press_handler, LV_EVENT_ALL, NULL);
        }

        configHasRun = true;
        set_var_prio_alert_freq("");
        queryDeviceInfo(pClient);
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
      gpsData.rawTime = convertToUnixTimestamp(gps);
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