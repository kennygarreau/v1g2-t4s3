#ifndef GPS_H
#define GPS_H

#include <ezTime.h>
#include <TinyGPSPlus.h>

void formatLocalTime(TinyGPSPlus &gps, char *buffer, size_t bufSize);
void formatLocalDate(TinyGPSPlus &gps, char *buffer, size_t bufSize);
uint32_t convertToUnixTimestamp(TinyGPSPlus &gps);
void gpsTask(void *parameter);

extern Timezone tz;
extern HardwareSerial gpsSerial;
extern TinyGPSPlus gps;

#endif