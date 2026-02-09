#ifndef V1_CONFIG_H
#define V1_CONFIG_H

#include <NimBLEDevice.h>
#include <Preferences.h>
#include "LilyGo_AMOLED.h"
#include "wifi.h"
#include <vector>

#define FIRMWARE_VERSION "1.6.0d"
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
extern bool proxyConnected, bt_connected, muted, alertPresent, v1le, savvy, remoteAudio;
extern bool wifiClientConnected, wifiConnecting, wifiConnected, localWifiStarted, webStarted;

extern int alertTableSize;
extern uint8_t currentSpeed;
extern SemaphoreHandle_t xWiFiLock;
extern SemaphoreHandle_t gpsDataMutex;
extern SemaphoreHandle_t bleMutex;

extern std::vector<std::pair<int, int>> sectionBounds;
extern std::vector<std::pair<int, int>> sweepBounds;

struct WiFiCredential {
    String ssid;
    String password;
};

// active, entrytype, timestamp, lastSeen, counter, latitude, longitude, speed, course, strength, direction, frequency
struct LockoutEntry {
    double latitude;
    double longitude;
    uint32_t timestamp;
    uint32_t lastSeen;

    int speed;
    int course;
    int strength;
    int direction; // 1: front, 2 side, 3: rear
    int frequency;

    uint8_t counter;
    bool active; // 0: inactive, 1: active
    bool entryType; // 0: auto 1: manual
};

/**
 * @brief create an alert log entry
 * 
 * @param timestamp uint32_t
 * @param latitude double
 * @param longitude double
 * @param frequency uint16_t
 * @param course uint16_t
 * @param speed uint8_t
 * @param strength uint8_t
 * @param direction uint8_t
 * @param padding uint8_t
 */
struct LogEntry {
    uint32_t timestamp;
    double latitude;
    double longitude;
    uint16_t frequency;
    uint16_t course;
    uint8_t speed;
    uint8_t strength;
    uint8_t direction;
    uint8_t padding;
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

struct RadarPacket {
    uint8_t data[16];
    size_t length;
};

extern QueueHandle_t radarQueue;

extern lockoutSettings autoLockoutSettings;

enum UnitSystem : uint8_t { METRIC = 0, IMPERIAL = 1 };

struct v1Settings {
  uint8_t displayOrientation;
  uint8_t brightness;
  uint32_t textColor;
  std::vector<WiFiCredential> wifiCredentials;
  String localSSID;
  String localPW;
  wifi_mode_t wifiMode;
  UnitSystem unitSystem;
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
    int kSensitivityBit0;
    int kSensitivityBit1;
    bool mrctPhoto;
    int xSensitivityBit0;
    int xSensitivityBit1;
    bool driveSafe3dPhoto;
    bool driveSafe3dHdPhoto;
    bool redflexHaloPhoto;
    bool redflexNK7Photo;
    bool ekinPhoto;
    bool photoVerifier;
    std::string xSensitivity;
    std::string kSensitivity;
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
  float altitude;
  float course;
  float hdop;
  uint8_t speed;
  int satelliteCount;
  char signalQuality[10];
  char date[11];
  char time[16];
  //char localtime[16];
  uint32_t rawTime;
  uint32_t ttffMs;
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
    int connectedWifiClients;
    int btStr;
    int wifiRSSI;
    float voltage;

    std::string boardType;
    uint8_t heapFrag;
    uint8_t boardRev;
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
    uint8_t barCount; // TODO: convert to array
    uint8_t freqCount;
    float frequencies[MAX_ALERTS + 1];
    std::string direction[MAX_ALERTS + 1];
};

#endif // V1_CONFIG_H