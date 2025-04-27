#ifndef V1_CONFIG_H
#define V1_CONFIG_H

#include <NimBLEDevice.h>
#include <Preferences.h>
#include "LilyGo_AMOLED.h"
#include "wifi.h"
#include <vector>

#define FIRMWARE_VERSION "1.2.2a"
#define BAUD_RATE 9600
#define FULLY_CHARGED_VOLTAGE 4124
#define EMPTY_VOLTAGE 3100
#define MAX_ALERTS 4
#define MAX_WIFI_NETWORKS 4

#define MUTING_RADIUS_KM 0.8
#define LAT_OFFSET 0.008 // Roughly 0.8 km in degrees latitude (~800m)
#define LON_OFFSET LAT_OFFSET
#define MAX_LOCATIONS 50000

//#define KBLOCK1 24.199 // +/- .005 (5 MHz) Honda
//#define KBLOCK2 24.168 // +/- .002 (2 MHz) Honda / Acura
//#define KBLOCK3 24.123 // +/- .002 (2 MHz) Mazda
//#define KBLOCK4 24.072 // +/- .003 (3 MHz) GM
//#define KBLOCK5 24.224 // +/- .002 (2 MHz) Honda

extern LilyGo_AMOLED amoled;
extern bool bt_connected, muted, alertPresent, v1le, savvy, remoteAudio;
extern bool wifiConnecting, wifiConnected, localWifiStarted, webStarted;

extern int alertTableSize;
extern uint8_t currentSpeed;
extern SemaphoreHandle_t xWiFiLock;
extern SemaphoreHandle_t gpsDataMutex;

extern std::vector<std::pair<int, int>> sectionBounds;
extern std::vector<std::pair<int, int>> sweepBounds;

struct WiFiCredential {
    String ssid;
    String password;
};

// active, entrytype, timestamp, lastSeen, counter, latitude, longitude, speed, course, strength, direction, frequency
struct LockoutEntry {
    bool active; // 0: inactive, 1: active
    bool entryType; // 0: auto 1: manual
    uint32_t timestamp;
    uint32_t lastSeen;
    uint8_t counter;
    double latitude;
    double longitude;
    int speed;
    int course;
    int strength;
    int direction; // 1: front, 2 side, 3: rear
    int frequency;
};

/**
 * @brief create an alert log entry
 * 
 * @param timestamp uint32_t
 * @param latitude double
 * @param longitude double
 * @param speed int
 * @param course int
 * @param strength int
 * @param direction bool
 * @param frequency int
 */
struct LogEntry {
    uint32_t timestamp;
    double latitude;
    double longitude;
    uint8_t speed;
    int course;
    int strength;
    int direction;
    int frequency;
};

//extern std::vector<LockoutEntry> savedLockoutLocations;

struct lockoutSettings {
    bool enable;
    int minThreshold;
    int learningTime;
    int requiredAlerts;
    int radius;
    bool setLockoutColor;
    uint32_t lockoutColor;
    int inactiveTime;
};

extern lockoutSettings autoLockoutSettings;

struct v1Settings {
  uint8_t displayOrientation;
  uint8_t brightness;
  uint32_t textColor;
  std::vector<WiFiCredential> wifiCredentials;
  String localSSID;
  String localPW;
  WiFiModeSetting wifiMode;
  String unitSystem;
  bool isPortraitMode;
  bool disableBLE;
  bool proxyBLE;
  bool useV1LE;
  bool enableGPS;
  bool enableWifi;
  bool displayTest;
  bool turnOffDisplay;
  bool onlyDisplayBTIcon;
  bool useDefaultV1Mode;
  bool showBogeyCount;
  uint8_t lowSpeedThreshold;
  bool muteToGray;
  bool colorBars;
  String timezone;
  uint8_t networkCount;
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
    bool customFreqEnabled;
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
    const char* defaultMode;
    uint8_t rawMode;
    int mainVolume;
    int mutedVolume;
    int sweepSections;
    int maxSweepIndex;
    std::vector<std::pair<int, int>> sections;
    std::vector<std::pair<int, int>> sweeps;
};

extern Config globalConfig;

struct GPSData {
  double latitude;
  double longitude;
  uint8_t speed;
  double altitude;
  double course;
  double hdop;
  int satelliteCount;
  String signalQuality;
  String time;
  String date;
  String localtime;
  uint32_t rawTime;
};

extern GPSData gpsData;
extern bool gpsAvailable;

struct Stats {
    unsigned long uptime;
    uint32_t cpuBusy;
    int cpuCores;
    uint32_t freeHeap;
    uint32_t freePsram;
    uint32_t totalHeap;
    uint32_t totalPsram;
    uint32_t totalStorageKB;
    uint32_t usedStorageKB;
    uint8_t heapFrag;
    std::string boardType;
    uint8_t boardRev;
    int connectedWifiClients;
    int btStr;
    int wifiRSSI;
    float voltage;
};

extern Stats stats;

/*
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
*/

//extern DisplayConstants selectedConstants;
extern std::string manufacturerName, modelNumber, serialNumber, softwareRevision;

extern Preferences preferences;
extern bool isVBusIn, batteryCharging, isPortraitMode;
extern unsigned long bootMillis;

// int alertCount, float frequencies[5], std::string direction[5], int barCount, int freqCount
struct AlertTableData {
    uint8_t alertCount; // this might be spurious
    float frequencies[MAX_ALERTS + 1];
    std::string direction[MAX_ALERTS + 1];
    uint8_t barCount; // TODO: convert to array
    uint8_t freqCount;
};

#endif // V1_CONFIG_H