#include <Arduino.h>
#include "gps.h"
#include "v1_config.h"
#include <TinyGPS++.h>

HardwareSerial gpsSerial(1);
TinyGPSPlus gps;
GPSData gpsData;
Timezone tz;
uint8_t currentSpeed = 0;
unsigned long lastValidGPSUpdate = 0;
bool firstFixRecorded;

SemaphoreHandle_t gpsDataMutex;

void formatLocalTime(TinyGPSPlus &gps, char *buffer, size_t bufSize)
{
  if (!gps.time.isValid())
  {
    snprintf(buffer, sizeof(bufSize), "00:00:00");
    return;
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

  snprintf(buffer, bufSize, "%02d:%02d:%02d", hour(localTime), minute(localTime), second(localTime));
}

void formatLocalDate(TinyGPSPlus &gps, char *buffer, size_t bufSize)
{
  if (!gps.date.isValid())
  {
    snprintf(buffer, bufSize, "00/00/0000");
    return;
  }

  tmElements_t tm;
  tm.Hour   = gps.time.hour();
  tm.Minute = gps.time.minute();
  tm.Second = gps.time.second();
  tm.Day    = gps.date.day();
  tm.Month  = gps.date.month();
  tm.Year   = gps.date.year() - 1970;

  time_t utcTime = makeTime(tm);
  time_t localTime = tz.tzTime(utcTime);

  snprintf(buffer, bufSize, "%02d/%02d/%04d", month(localTime), day(localTime), year(localTime));
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
  uint32_t gpsStartMs = millis();
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
        if (!firstFixRecorded) {
          firstFixRecorded = true;
          gpsData.ttffMs = millis() - gpsStartMs;
          Serial.printf("Time to first GPS fix: %lu ms\n", gpsData.ttffMs);
        }

        if (xSemaphoreTake(gpsDataMutex, portMAX_DELAY)) {
          gpsData.latitude = gps.location.lat();
          gpsData.longitude = gps.location.lng();
          gpsData.satelliteCount = gps.satellites.value();
          gpsData.course = gps.course.deg();
          gpsData.rawTime = convertToUnixTimestamp(gps);

          char dateBuf[11];
          formatLocalDate(gps, dateBuf, sizeof(dateBuf));
          strncpy(gpsData.date, dateBuf, sizeof(gpsData.date));
          
          char timeBuf[16];
          formatLocalTime(gps, timeBuf, sizeof(timeBuf));
          strncpy(gpsData.time, timeBuf, sizeof(gpsData.time));
          gpsData.time[sizeof(gpsData.time) - 1] = '\0';

          gpsData.hdop = static_cast<float>(gps.hdop.value()) / 100.0;
          if (gpsData.hdop < 2) strcpy(gpsData.signalQuality, "excellent");
          else if (gpsData.hdop <= 5) strcpy(gpsData.signalQuality, "good");
          else if (gpsData.hdop <= 10) strcpy(gpsData.signalQuality, "moderate");
          else strcpy(gpsData.signalQuality, "poor");

          if (settings.unitSystem == METRIC)
          {
            gpsData.speed = static_cast<uint8_t>(round(gps.speed.kmph()));
            gpsData.altitude = gps.altitude.meters();
            currentSpeed = gpsData.speed;
          }
          else
          {
            gpsData.speed = static_cast<uint8_t>(round(gps.speed.mph()));
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
    vTaskDelay(pdMS_TO_TICKS(50));
    //Serial.printf("GPS Stack High Water: %u\n", uxTaskGetStackHighWaterMark(NULL));
  }
}
