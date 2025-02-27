//#pragma once
#ifndef V1_CONFIG_H
#define V1_CONFIG_H

#include "BLEDevice.h"
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include "LilyGo_AMOLED.h"
#include "wifi.h"
#include "v1_fs.h"

#define FIRMWARE_VERSION "1.0.1c"
#define BAUD_RATE 9600
#define WIFI_MODE WIFI_STA
#define FULLY_CHARGED_VOLTAGE 4124
#define EMPTY_VOLTAGE 3100
#define MAX_ALERTS 4
#define MAX_WIFI_NETWORKS 4
#define BLE_RETRY_INTERVAL 10000

#define EARTH_RADIUS_KM 6371.0
#define MUTING_RADIUS_KM 0.4
#define LAT_OFFSET 0.004 // Roughly 0.4 km in degrees latitude (~400m)
#define LON_OFFSET LAT_OFFSET
#define MAX_LOCATIONS 512

struct LockoutEntry {
    uint32_t timestamp;
    double latitude;
    double longitude;
};

extern LockoutEntry savedLockoutLocations[MAX_LOCATIONS];

extern BLEClient* pClient;
extern BLERemoteCharacteristic* clientWriteCharacteristic;
extern LilyGo_AMOLED amoled;
extern bool bt_connected;
extern bool muted;
extern bool alertPresent;
extern bool wifiConnecting;
extern bool wifiConnected;
extern unsigned long wifiStartTime;

struct WiFiCredential {
    String ssid;
    String password;
};

struct v1Settings {
  int displayOrientation;
  uint8_t brightness;
  uint32_t textColor;
  std::vector<WiFiCredential> wifiCredentials;
  String localSSID;
  String localPW;
  WiFiModeSetting wifiMode;
  String unitSystem;
  bool isPortraitMode;
  bool disableBLE;
  bool enableGPS;
  bool enableWifi;
  bool displayTest;
  bool turnOffDisplay;
  bool onlyDisplayBTIcon;
  int lowSpeedThreshold;
  String timezone;
  int networkCount;
};

extern v1Settings settings;

struct Config {
    bool xBand;
    bool kBand;
    bool kaBand;
    bool laserBand;
    std::string muteTo;
    bool bogeyLockLoud;
    bool rearMute;
    bool kuBand;
    bool euro;
    bool kVerifier;
    bool rearLaser;
    bool customFreqDisabled;
    bool kaAlwaysPrio;
    bool fastLaserDetection;
    int kaSensitivityBit0;
    int kaSensitivityBit1;
    std::string kaSensitivity;
    bool startupSequence;
    bool restingDisplay;
    bool bsmPlus;
    int autoMuteBit0;
    int autoMuteBit1;
    std::string autoMute;
    const char* mode;
};

extern Config globalConfig;

struct GPSData {
  double latitude;
  double longitude;
  double speed;
  double altitude;
  double course;
  double hdop;
  int satelliteCount;
  String signalQuality;
  String time;
  String date;
  String localtime;
  uint32_t rawTime;
  uint32_t cpuBusy;
  uint32_t freeHeap;
  uint32_t freePsram;
  uint32_t totalHeap;
  uint32_t totalPsram;
  uint32_t totalStorageKB;
  uint32_t usedStorageKB;
  int connectedClients;
  int btStr;
};

extern GPSData gpsData;
extern bool gpsAvailable;

struct DisplayConstants {
    int MAX_X;
    int MAX_Y;
    int MHZ_DISP_X;
    int MHZ_DISP_Y;
    int MHZ_DISP_Y_OFFSET;

    int BAR_START_X;
    int BAR_START_Y;

    int ARROW_FRONT_X;
    int ARROW_FRONT_Y;
    int ARROW_FRONT_WIDTH;

    int SMALL_ARROW_FRONT_X;
    int SMALL_ARROW_Y;
    int SMALL_ARROW_FRONT_WIDTH;

    int ARROW_SIDE_X;
    int ARROW_SIDE_Y;
    int ARROW_SIDE_WIDTH;
    int ARROW_SIDE_HEIGHT;

    int SMALL_ARROW_SIDE_X;
    int SMALL_ARROW_SIDE_Y;
    int SMALL_ARROW_SIDE_WIDTH;
    int SMALL_ARROW_SIDE_HEIGHT;

    int ARROW_REAR_X;
    int ARROW_REAR_Y;
    int ARROW_REAR_WIDTH;

    int SMALL_ARROW_REAR_X;
    int SMALL_ARROW_REAR_Y;
    int SMALL_ARROW_REAR_WIDTH;

    int BAR_WIDTH;
    int BAR_HEIGHT;
    int H_BAR_HEIGHT;
    int H_BAR_WIDTH;
    int SPACING;
};

//extern DisplayConstants selectedConstants;
extern std::string manufacturerName, modelNumber, serialNumber, hardwareRevision, firmwareRevision, softwareRevision;

extern Preferences preferences;
extern bool isVBusIn, batteryCharging, isPortraitMode;
extern uint16_t vBusVoltage, batteryVoltage;
extern float voltageInMv, batteryPercentage;

struct AlertTableData {
    int alertCount;
    float frequencies[MAX_ALERTS];
    std::string direction[MAX_ALERTS];
    int barCount;
    int freqCount;
};

// Custom UI color; should be modified to an acceptable TFT_COLOR
// See https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_eSPI.h for default enumerations
#define UI_COLOR TFT_RED

//#define V1LE
#ifdef V1LE
    // to-do: discover Bluetooth LE dongle Service UUID
    static BLEUUID v1leServiceUUID("92A0AFF4-9E05-11E2-AA59-F23C91AEC05E");
#else
    static BLEUUID bmeServiceUUID("92A0AFF4-9E05-11E2-AA59-F23C91AEC05E");
#endif

// Bluetooth service configurations
static BLEUUID deviceInfoUUID("180A");

// Device Information Service characteristics
// static BLEUUID manufacturerUUID("2A29");
// static BLEUUID modelUUID("2A24");
// static BLEUUID serialUUID("2A25");
// static BLEUUID hardwareUUID("2A27");
// static BLEUUID firmwareUUID("2A26");
// static BLEUUID softwareUUID("2A28");

// V1 Service characteristics
static BLEUUID infDisplayDataUUID("92A0B2CE-9E05-11E2-AA59-F23C91AEC05E"); // V1 out client in SHORT - notify for alerts
//static BLEUUID char2UUID("92A0B4E0-9E05-11E2-AA59-F23C91AEC05E"); // V1 out client in LONG
//static BLEUUID char5UUID("92A0BCE0-9E05-11E2-AA59-F23C91AEC05E"); // notify
static BLEUUID clientWriteUUID("92A0B6D4-9E05-11E2-AA59-F23C91AEC05E"); // client out V1 in SHORT
//static BLEUUID char4UUID("92A0B8D2-9E05-11E2-AA59-F23C91AEC05E"); // client out V1 in LONG
//static BLEUUID char6UUID("92A0BAD4-9E05-11E2-AA59-F23C91AEC05E"); // write and write without response 

#endif