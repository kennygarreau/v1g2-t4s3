#ifndef BLE_H
#define BLE_H

#include <NimBLEDevice.h>

extern bool serialReceived, versionReceived, volumeReceived, userBytesReceived, 
            sweepSectionsReceived, maxSweepIndexReceived, allSweepDefinitionsReceived;
extern bool bleInit, newDataAvailable;
extern std::string latestHexData;
extern std::string hexData, lastPacket;

void initBLE();
void initBLEServer();
void requestSerialNumber();
void requestVersion();
void requestVolume();
void requestUserBytes();
void requestSweepSections();
void requestMaxSweepIndex();
void requestAllSweepDefinitions();
void reqVolume();
void reqBatteryVoltage();
void requestMute();
void reqMuteOff();
void queryDeviceInfo(NimBLEClient* pClient);
void displayReader(NimBLEClient* pClient);

#endif