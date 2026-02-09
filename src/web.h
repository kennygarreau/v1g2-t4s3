#ifndef WEB_H
#define WEB_H

#include <Arduino.h>

enum LockoutField {
    ACTIVE,
    ENTRY_TYPE,
    TIMESTAMP,
    LAST_SEEN,
    COUNTER,
    LATITUDE,
    LONGITUDE,
    SPEED,
    COURSE,
    STRENGTH,
    DIRECTION,
    FREQUENCY,
    LOCKOUT_FIELD_COUNT // Total number of fields
};

enum LogField {
    TS,
    LAT,
    LON,
    SPD,
    CRSE,
    STR,
    DIR,
    FREQ,
    LOG_FIELD_COUNT
};

//extern const char *lockoutFieldNames[];

extern AsyncWebServer server;
extern unsigned long rebootTime;
extern bool isRebootPending;
extern const char *lockoutFieldNames[];
extern const char *logFieldNames[];

void systemManagerTask(void *pvParameters);
void setupWebServer();
void checkReboot();
void getDeviceStats();
void bootDeviceStats();
String readFileFromSPIFFS(const char* path);

#endif
