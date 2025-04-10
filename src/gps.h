#ifndef GPS_H
#define GPS_H

#include <ezTime.h>
#include <TinyGPSPlus.h>

const char* formatLocalTime(TinyGPSPlus &gps);
const char* formatLocalDate(TinyGPSPlus &gps);
uint32_t convertToUnixTimestamp(TinyGPSPlus &gps);
void gpsTask(void *parameter);

extern Timezone tz;
extern HardwareSerial gpsSerial;
extern TinyGPSPlus gps;

#endif