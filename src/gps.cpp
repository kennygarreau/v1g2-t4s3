#include <Arduino.h>
#include <ezTime.h>
#include "gps.h"
#include "v1_config.h"
#include <TinyGPS++.h>

const char* formatLocalTime(TinyGPSPlus &gps) {
  static char timeBuffer[10];

  if (!gps.time.isValid()) {
    snprintf(timeBuffer, sizeof(timeBuffer), "00:00:00");
    return timeBuffer;
  }

  tmElements_t tm;
  tm.Hour = gps.time.hour();
  tm.Minute = gps.time.minute();
  tm.Second = gps.time.second();
  tm.Day = gps.date.day();
  tm.Month = gps.date.month();
  tm.Year = gps.date.year() - 1970;

  time_t utcTime = makeTime(tm);
  time_t localTime = tz.tzTime(utcTime, LOCAL_TIME);

  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", hour(localTime), minute(localTime), second(localTime));

  return timeBuffer;
}
  
const char* formatLocalDate(TinyGPSPlus &gps) {
  static char dateBuffer[11];
  
  if (!gps.date.isValid()) {
    snprintf(dateBuffer, sizeof(dateBuffer), "00/00/0000");
    return dateBuffer;
  }

  tmElements_t tm;
  tm.Hour = gps.time.hour();
  tm.Minute = gps.time.minute();
  tm.Second = gps.time.second();
  tm.Day = gps.date.day();
  tm.Month = gps.date.month();
  tm.Year = gps.date.year() - 1970;

  time_t utcTime = makeTime(tm);
  time_t localTime = tz.tzTime(utcTime);

  snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d", month(localTime), day(localTime), year(localTime));

  return dateBuffer;
}
  
uint32_t convertToUnixTimestamp(TinyGPSPlus &gps) {
  TinyGPSDate gpsDate = gps.date;
  TinyGPSTime gpsTime = gps.time;

  tm timeInfo = {};
  
  timeInfo.tm_year = gpsDate.year() - 1900;
  timeInfo.tm_mon = gpsDate.month() - 1;
  timeInfo.tm_mday = gpsDate.day();
  timeInfo.tm_hour = gpsTime.hour();
  timeInfo.tm_min = gpsTime.minute();
  timeInfo.tm_sec = gpsTime.second();

  uint32_t unixTimestamp = mktime(&timeInfo);

  return unixTimestamp;
}