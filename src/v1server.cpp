/*
 * Valentine V1 Gen2 Remote Display
 * version: v1.0
 * Author: Kenny G
 * Date: February 2025
 * License: MIT
 */

//#include <Arduino.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include "ble.h"
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
#include <Ticker.h>

Ticker writeBatteryVoltageTicker;
Ticker writeVolumeTicker;

AsyncWebServer server(80);
static SemaphoreHandle_t xWiFiLock = NULL;

static bool laserAlert = false;
static std::string bogeyValue, barValue, bandValue, directionValue;
std::vector<LockoutEntry> *lockoutList;

LilyGo_AMOLED amoled;

GPSData gpsData;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
int currentSpeed = 0;
float batteryPercentage = 0.0f;
bool batteryConnected, batteryCharging, isVBusIn, wifiConnecting;
float voltageInMv = 0.0f;
uint16_t vBusVoltage = 0;
bool gpsAvailable = false;
bool wifiConnected = false;
unsigned long lastValidGPSUpdate = 0;
unsigned long lastBLEAttempt = 0;
bool v1le = false;

v1Settings settings;
lockoutSettings autoLockoutSettings;
Preferences preferences;
Timezone tz;

int loopCounter = 0;
unsigned long bootMillis = 0;
unsigned long lastMillis = 0;
const unsigned long uiTickInterval = 5;

SPIFFSFileManager fileManager;

void requestMute() {
  if (!settings.displayTest && clientWriteCharacteristic) {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqMuteOn(), 7, false);
  }
}

void loadLockoutSettings() {
  preferences.begin("lockoutSettings", false);
  autoLockoutSettings.minThreshold = preferences.getInt("minThreshold", 3);
  autoLockoutSettings.enable = preferences.getBool("enable", true);
  autoLockoutSettings.learningTime = preferences.getInt("learningTime", 24);
  autoLockoutSettings.lockoutColor = preferences.getUInt("lockoutColor", 0xBFBBA9);
  autoLockoutSettings.setLockoutColor = preferences.getBool("setLockoutColor", true);
  autoLockoutSettings.requiredAlerts = preferences.getInt("requiredAlerts", 3);
  autoLockoutSettings.radius = preferences.getInt("lockoutRadius", 800);
  preferences.end();
}

void loadSettings() {
  preferences.begin("settings", false);
  settings.brightness = preferences.getUInt("brightness", amoled.getBrightness());

  int mode = preferences.getInt("wifiMode", WIFI_SETTING_STA);
  if (mode < WIFI_SETTING_AP || mode > WIFI_SETTING_APSTA) {
      mode = WIFI_SETTING_STA;
  }
  settings.wifiMode = static_cast<WiFiModeSetting>(mode);

  settings.localSSID = preferences.getString("localSSID", "v1display");
  settings.localPW = preferences.getString("localPW", "password123");
  settings.disableBLE = preferences.getBool("disableBLE", false);
  settings.useV1LE = preferences.getBool("useV1LE", false);
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
  settings.useDefaultV1Mode = preferences.getBool("useDefMode", false);
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
  preferences.end();

  loadLockoutSettings();
}

void setup()
{
  bootMillis = millis();
  Serial.begin();
  analogReadResolution(12);
  delay(500);

  Serial.println("Reading initial settings...");
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

  lockoutList = (std::vector<LockoutEntry> *)ps_malloc(sizeof(std::vector<LockoutEntry>));
  if (!lockoutList) {
    Serial.println("Failed to allocate lockoutList in PSRAM");
    return;
  }

  new (lockoutList) std::vector<LockoutEntry>();
  Serial.println("Lockout list allocated in PSRAM.");
  Serial.printf("Setup running on core %d\n", xPortGetCoreID());

  if (settings.enableGPS) {
    Serial.println("Initializing GPS...");
    gpsSerial.begin(BAUD_RATE, SERIAL_8N1, RXD, TXD);
  }

  if (!fileManager.init()) {
    Serial.println("Failed to initialize SPIFFS");
    return;
  }

  //fileManager.testWrite();

  if (!fileManager.openDatabase()) return;
  fileManager.createTable();
  fileManager.readLockouts();

  ui_init();
  ui_tick();
  lv_task_handler();

 if (!settings.disableBLE && !settings.displayTest) {
    initBLE();
  }

  // xWiFiLock =  xSemaphoreCreateBinary();
  // xSemaphoreGive( xWiFiLock );

  wifiSetup();
  setupWebServer();

  Serial.println("v1g2 firmware version: " + String(FIRMWARE_VERSION));

  lv_obj_t * scr = lv_scr_act();
  if (scr) {
    lv_obj_add_event_cb(scr, main_press_handler, LV_EVENT_ALL, NULL);
  }
  gpsData.totalHeap = ESP.getHeapSize();
  gpsData.totalPsram = ESP.getPsramSize();
  gpsData.totalStorageKB = fileManager.getStorageTotal();
  gpsData.usedStorageKB = fileManager.getStorageUsed();

  writeVolumeTicker.attach(61, reqVolume);
  writeBatteryVoltageTicker.attach(10, reqBatteryVoltage);
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

  // in case we get disconnected for a while - this will disconnect the AP client so we should not do this frequently
  /*
  if (settings.wifiMode == WIFI_SETTING_STA && WiFi.getMode() == WIFI_MODE_AP) {
    if (millis() - lastWifiReconnect > 60000) {
      Serial.println("WiFi lost. Reconnecting...");
      wifiScan();
      lastWifiReconnect = millis();
    }
  }
  */

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

  // decode loop takes ~2ms; could this be moved to a callback?
  std::string packet = hexData.c_str();
  if (packet != lastPacket) {
    PacketDecoder decoder(packet);
    std::string decoded = decoder.decode(settings.lowSpeedThreshold, currentSpeed);
    lastPacket = packet;
  } 
  
  unsigned long currentMillis = millis();

  if (currentMillis - lastMillis >= 2000) {
    unsigned long uptime = (millis() - bootMillis) / 1000;
    Serial.printf("Uptime: %u | Loops executed: %d\n", uptime, loopCounter); // uncomment for loop profiling
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
    if (bt_connected && clientWriteCharacteristic) {
      gpsData.btStr = getBluetoothSignalStrength();
    }
    
    batteryCharging = amoled.isCharging();
    uint16_t espVoltage = amoled.getBattVoltage();
    voltageInMv = espVoltage; // cast this to float
    batteryPercentage = ((voltageInMv - EMPTY_VOLTAGE) / (FULLY_CHARGED_VOLTAGE - EMPTY_VOLTAGE)) * 100.0;
    batteryPercentage = constrain(batteryPercentage, 0, 100);
    uint32_t cpuIdle = lv_timer_get_idle();  
    gpsData.cpuBusy = 100 - cpuIdle;
    gpsData.freeHeap = ESP.getFreeHeap();
    gpsData.freePsram = ESP.getFreePsram();

    // This will obtain the User-defined settings (eg. disabling certain bands)
    if (!configHasRun && !settings.displayTest) {
      if (bt_connected && clientWriteCharacteristic) {
        if (!serialReceived) {
          Serial.println("Awaiting Serial Number...");
          requestSerialNumber();
        }
        if (!versionReceived) {
          Serial.println("Awaiting Version...");
          requestVersion();
        }
        if (!volumeReceived) {
          Serial.println("Awaiting Volume Settings...");
          requestVolume();
        }
        if (!userBytesReceived) {
          Serial.println("Awaiting User Bytes...");
          requestUserBytes();
        }
      }
    }

    if (serialReceived && versionReceived && volumeReceived && userBytesReceived) {
      Serial.println("All device information received!");
      configHasRun = true;
      set_var_prio_alert_freq("");

      if (!v1le) {
        queryDeviceInfo(pClient);
      }
  
      serialReceived = false;
      versionReceived = false;
      volumeReceived = false;
      userBytesReceived = false;
    }
    
    if (!sweepSectionsReceived && !manufacturerName.empty()) {
      requestSweepSections();
    }
    if (!maxSweepIndexReceived && sweepSectionsReceived) {
      requestMaxSweepIndex();
    }
    if (!allSweepDefinitionsReceived && maxSweepIndexReceived) {
      requestAllSweepDefinitions();
    }

    lastMillis = currentMillis;
    loopCounter = 0;
    checkReboot();
  }
  
  if (settings.enableGPS && (currentMillis - lastGPSUpdate >= 200)) {
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
    lv_task_handler();
  }

  loopCounter++;
  delayMicroseconds(10);
  yield();
}