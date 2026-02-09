#ifndef BLE_H
#define BLE_H

#include <NimBLEDevice.h>

extern NimBLEClient* pClient;
extern NimBLERemoteCharacteristic* clientWriteCharacteristic;

extern bool serialReceived, versionReceived, volumeReceived, userBytesReceived, 
            sweepSectionsReceived, maxSweepIndexReceived, allSweepDefinitionsReceived;
extern bool bleInit, newDataAvailable;
extern std::vector<uint8_t> latestRawData;

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
void onProxyReady();

void volumeTask(void *p);
void batteryTask(void *p);

// Bluetooth service configurations
static NimBLEUUID deviceInfoUUID("180A");
static NimBLEUUID bmeServiceUUID("92A0AFF4-9E05-11E2-AA59-F23C91AEC05E");

// Device Information Service characteristics
// static BLEUUID manufacturerUUID("2A29");
// static BLEUUID modelUUID("2A24");
// static BLEUUID serialUUID("2A25");
// static BLEUUID hardwareUUID("2A27");
// static BLEUUID firmwareUUID("2A26");
// static BLEUUID softwareUUID("2A28");

// V1 Service characteristics
static NimBLEUUID    infDisplayDataUUID("92A0B2CE-9E05-11E2-AA59-F23C91AEC05E"); // V1 out client in SHORT - notify for alerts
static NimBLEUUID     clientLongOutUUID("92A0B4E0-9E05-11E2-AA59-F23C91AEC05E"); // V1 out client in LONG
static NimBLEUUID infDisplayDataAltUUID("92A0BCE0-9E05-11E2-AA59-F23C91AEC05E"); // notify
static NimBLEUUID       clientWriteUUID("92A0B6D4-9E05-11E2-AA59-F23C91AEC05E"); // client out V1 in SHORT
static NimBLEUUID   clientWriteLongUUID("92A0B8D2-9E05-11E2-AA59-F23C91AEC05E"); // client out V1 in LONG
static NimBLEUUID      writewithoutUUID("92A0BAD4-9E05-11E2-AA59-F23C91AEC05E"); // write and write without response 

#endif