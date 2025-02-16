#include <Arduino.h>
#include "gps.h"
#include "v1_config.h"
#include <ezTime.h>
#include <TinyGPS++.h>

String formatLocalTime(TinyGPSPlus &gps) {
    if (!gps.time.isValid()) {
      return "00:00:00";
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

    char timeBuffer[10];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", hour(localTime), minute(localTime), second(localTime));
  
    return String(timeBuffer);
}
  
String formatLocalDate(TinyGPSPlus &gps) {
    if (!gps.date.isValid()) {
      return "00/00/0000";
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
  
    char dateBuffer[11];
    snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d", month(localTime), day(localTime), year(localTime));
  
    return String(dateBuffer);
  }
  