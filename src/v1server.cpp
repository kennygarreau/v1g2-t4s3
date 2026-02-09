/*
 * Valentine V1 Gen2 Remote Display
 * version: v1.0
 * Author: Kenny G
 * Date: February 2025
 * License: MIT
 */

#include "v1_config.h"
#include <LV_Helper.h>
#include <FreeRTOS.h>
#include "ble.h"
#include "v1_packet.h"
#include "v1_fs.h"
#include "web.h"
#include <ui/ui.h>
#include "utils.h"
#include "gps.h"
#include "esp_flash.h"

AsyncWebServer server(80);
//SemaphoreHandle_t xWiFiLock = NULL;

static bool laserAlert = false;
std::vector<LockoutEntry> *lockoutList;

LilyGo_AMOLED amoled;

bool batteryConnected, batteryCharging, isVBusIn, wifiConnecting, webStarted;
bool gpsAvailable, wifiConnected, v1le;

v1Settings settings;
lockoutSettings autoLockoutSettings;
Preferences preferences;
Stats stats;

int loopCounter = 0;
unsigned long bootMillis = 0;
unsigned long lastMillis = 0;
unsigned long lastWifiReconnect = 0;
const unsigned long uiTickInterval = 16;
const unsigned long atTickInterval = 250;

QueueHandle_t radarQueue;

SPIFFSFileManager fileManager;

void loadLockoutSettings() {
  preferences.begin("lockoutSettings", false);
  autoLockoutSettings.minThreshold = preferences.getInt("minThreshold", 3);
  autoLockoutSettings.enable = preferences.getBool("enable", false);
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

  int mode = preferences.getInt("wifiMode", WIFI_MODE_STA);
  if (mode < WIFI_MODE_AP || mode > WIFI_MODE_APSTA) {
      mode = WIFI_MODE_STA;
  }
  settings.wifiMode = static_cast<wifi_mode_t>(mode);

  settings.localSSID = preferences.getString("localSSID", "v1display");
  settings.localPW = preferences.getString("localPW", "password123");
  settings.disableBLE = preferences.getBool("disableBLE", false);
  settings.proxyBLE = preferences.getBool("proxyBLE", true);
  settings.useV1LE = preferences.getBool("useV1LE", false);
  settings.displayTest = preferences.getBool("displayTest", false);
  settings.enableGPS = preferences.getBool("enableGPS", false);
  settings.enableWifi = preferences.getBool("enableWifi", true);
  settings.lowSpeedThreshold = preferences.getInt("lowSpeedThres", 35);
  
  String unitStr = preferences.getString("unitSystem", "Imperial"); // default = "Imperial"
  if (unitStr == "Metric") {
    settings.unitSystem = METRIC;
  } else {
    settings.unitSystem = IMPERIAL;
  }
  settings.displayOrientation = preferences.getInt("displayOrient", 0);
  settings.timezone = preferences.getString("timezone", "UTC");
  settings.muteToGray = preferences.getBool("muteToGray", false);
  settings.colorBars = preferences.getBool("colorBars", true);
  settings.showBogeyCount = preferences.getBool("showBogeys", false);

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
  uint8_t networkCount = preferences.getInt("networkCount", 0);

  for (uint8_t i = 0; i < networkCount; i++) {
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
  pinMode(8, INPUT_PULLUP);
  analogReadResolution(12);
  
  Serial.printf("Heap at boot: %u\n", ESP.getFreeHeap());
  Serial.println("Reading initial settings...");
  loadSettings();

  if (!settings.disableBLE && !settings.displayTest) {
    initBLE();
    Serial.printf("Free heap after BLE init: %u\n", ESP.getFreeHeap());
  }

  Serial.println("v1g2 firmware version: " + String(FIRMWARE_VERSION));

  if (settings.enableWifi) {
    wifiSetup();
    Serial.printf("Free heap after Wifi startup: %u\n", ESP.getFreeHeap());
  }

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

  Serial.printf("Free heap after board init: %u\n", ESP.getFreeHeap());

  beginLvglHelperDMA(amoled);
  Serial.printf("Setup running on core %d\n", xPortGetCoreID());
  Serial.printf("Free heap after LVGL init: %u\n", ESP.getFreeHeap());

  ui_init();
  ui_tick();
  lv_task_handler();

  if (!initStorage()) {
    Serial.println("Failed to initialize LittleFS");
    return;
  }

  Serial.printf("Free heap after LittleFS init: %u\n", ESP.getFreeHeap());

  lockoutList = (std::vector<LockoutEntry> *)ps_malloc(sizeof(std::vector<LockoutEntry>));
  if (!lockoutList) {
    Serial.println("Failed to allocate lockoutList in PSRAM");
    return;
  }

  new (lockoutList) std::vector<LockoutEntry>();
  Serial.println("Lockout list allocated in PSRAM.");

  // if (!fileManager.openDatabase()) return;
  // fileManager.createTable();
  // fileManager.readLockouts();

  // Serial.printf("Free heap after DB startup: %u\n", ESP.getFreeHeap());

  gpsDataMutex = xSemaphoreCreateMutex();

  if (settings.enableGPS) {
    Serial.println("Initializing GPS...");
    gpsSerial.begin(BAUD_RATE, SERIAL_8N1, RXD, TXD);
    xTaskCreatePinnedToCore(gpsTask, "GPSTask", 3072, NULL, 1, NULL, 1);
  }

  lv_obj_t * scr = lv_scr_act();
  if (scr) {
    lv_obj_add_event_cb(scr, main_press_handler, LV_EVENT_ALL, NULL);
  }
  
  bootDeviceStats();
  stats.totalStorageKB = fileManager.getStorageTotal();
  stats.usedStorageKB = fileManager.getStorageUsed();

  radarQueue = xQueueCreate(10, sizeof(RadarPacket));

  if (settings.displayTest) {
    xTaskCreate(processingTask, "ProcTask", 4096, NULL, 2, NULL);
    xTaskCreate(displayTestTask, "DisplayTest", 2048, NULL, 1, NULL);
  }

  xTaskCreatePinnedToCore(systemManagerTask, "SystemMgr", 4096, NULL, 1, NULL, 1);

  unsigned long elapsedMillis = millis() - bootMillis;
  Serial.printf("setup finished: %.2f seconds\n", elapsedMillis / 1000.0);
}

void loop() {  
  // unsigned long loopStart = millis();
  // unsigned long now = loopStart;
  
  static bool configHasRun = false;
  static unsigned long lastTick = 0;
  static unsigned long lastTableTick = 0;
  
  if (bt_connected && bleInit) {
    displayReader(pClient);
    bleInit = false;

    unsigned long elapsedMillis = millis() - bootMillis;
    Serial.printf("processing packets at: %.2f seconds\n", elapsedMillis / 1000.0);
  }

  if (newDataAvailable) {
    newDataAvailable = false;

    PacketDecoder decoder(latestRawData);
    std::string decoded = decoder.decode_v2(settings.lowSpeedThreshold, currentSpeed);
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= 2000) {
    //Serial.printf("Uptime: %u | Loops executed: %d\n", stats.uptime, loopCounter); // uncomment for loop profiling
    getDeviceStats();

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

  static uint32_t last_band_check = 0;
  static uint32_t last_status_print = 0;
  if (millis() - last_band_check > 100 && bt_connected) {
    checkBandTimeouts();
    last_band_check = millis();
    if (millis() - last_status_print > 1000 && activeBands != 0x00) {
      Serial.printf("⏱️ Band Status: Ka=%d K=%d X=%d Laser=%d | activeBands=0x%02X\n",
                    ka_state.active, k_state.active, x_state.active, laser_state.active,
                    activeBands);
      last_status_print = millis();
    }
  }

  unsigned long now = millis();

  // TODO: move this to a task
  if (now - lastTableTick >= atTickInterval) {
    lastTableTick = now;
    tick_alertTable();
  }

  if (now - lastTick >= uiTickInterval) {    
    lastTick = now;
    ui_tick();
    lv_task_handler();
    unsigned long elapsedHandler = millis() - now;
    if (elapsedHandler > 16) {
      Serial.printf("Warning: screen draw time: %u ms\n", elapsedHandler);
    }
  }

  loopCounter++;
}
