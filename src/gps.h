#ifndef GPS_H
#define GPS_H

#include <ezTime.h>
#include <TinyGPSPlus.h>

String formatLocalTime(TinyGPSPlus &gps);
String formatLocalDate(TinyGPSPlus &gps);
uint32_t convertToUnixTimestamp(TinyGPSPlus &gps);
extern Timezone tz;

#endif