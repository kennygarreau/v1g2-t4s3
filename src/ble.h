#ifndef BLE_H
#define BLE_H

#include <NimBLEDevice.h>

extern bool serialReceived, versionReceived, volumeReceived, userBytesReceived, 
            sweepSectionsReceived, maxSweepIndexReceived, allSweepDefinitionsReceived;
extern bool bleInit;
extern std::string hexData, lastPacket;

void initBLE();
void requestSerialNumber();
void requestVersion();
void requestVolume();
void requestUserBytes();
void requestSweepSections();
void requestMaxSweepIndex();
void requestAllSweepDefinitions();
void queryDeviceInfo(NimBLEClient* pClient);
void displayReader(NimBLEClient* pClient);

#endif