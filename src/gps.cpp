#include <Arduino.h>
#include <ezTime.h>
#include "gps.h"
#include "v1_config.h"
#include <TinyGPS++.h>

HardwareSerial gpsSerial(1);
TinyGPSPlus gps;
GPSData gpsData;
int currentSpeed = 0;
unsigned long lastValidGPSUpdate = 0;

SemaphoreHandle_t gpsDataMutex;

const char *formatLocalTime(TinyGPSPlus &gps)
{
  static char timeBuffer[10];

  if (!gps.time.isValid())
  {
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

const char *formatLocalDate(TinyGPSPlus &gps)
{
  static char dateBuffer[11];

  if (!gps.date.isValid())
  {
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

uint32_t convertToUnixTimestamp(TinyGPSPlus &gps)
{
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

void gpsTask(void *parameter)
{
  for (;;)
  {
    if (settings.enableGPS)
    {
      while (gpsSerial.available() > 0)
      {
        char c = gpsSerial.read();
        // Serial.print(c);
        gps.encode(c);
      }
      if (gps.location.isUpdated() && gps.location.isValid()) {
        if (xSemaphoreTake(gpsDataMutex, portMAX_DELAY)) {
          gpsData.latitude = gps.location.lat();
          gpsData.longitude = gps.location.lng();
          gpsData.satelliteCount = gps.satellites.value();
          gpsData.course = gps.course.deg();
          gpsData.rawTime = convertToUnixTimestamp(gps);
          gpsData.date = formatLocalDate(gps);
          gpsData.time = formatLocalTime(gps);
          gpsData.hdop = static_cast<double>(gps.hdop.value()) / 100.0;

          gpsData.signalQuality = (gpsData.hdop < 2) ? "excellent" : (gpsData.hdop <= 5) ? "good"
                                                                : (gpsData.hdop <= 10)  ? "moderate"
                                                                                        : "poor";

          if (settings.unitSystem == "Metric")
          {
            gpsData.speed = static_cast<int>(round(gps.speed.kmph()));
            gpsData.altitude = gps.altitude.meters();
            currentSpeed = gpsData.speed;
          }
          else
          {
            gpsData.speed = static_cast<int>(round(gps.speed.mph()));
            gpsData.altitude = gps.altitude.feet();
            currentSpeed = gpsData.speed;
          }
          xSemaphoreGive(gpsDataMutex);
        }
        gpsAvailable = true;
        lastValidGPSUpdate = millis();
      }

      if (millis() - lastValidGPSUpdate > 5000)
      {
        gpsAvailable = false;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
