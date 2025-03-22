#ifndef WEB_H
#define WEB_H

#include <Arduino.h>

extern AsyncWebServer server;
extern unsigned long rebootTime;
extern bool isRebootPending;
extern const char *lockoutFieldNames[];

void setupWebServer();
void checkReboot();
String readFileFromSPIFFS(const char* path);

#endif
