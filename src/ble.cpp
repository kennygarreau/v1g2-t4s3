#include "ble.h"
#include "v1_packet.h"
#include "v1_config.h"

bool serialReceived = false;
bool versionReceived = false;
bool volumeReceived = false;
bool userBytesReceived = false;
bool sweepSectionsReceived = false;
bool maxSweepIndexReceived = false;
bool allSweepDefinitionsReceived = false;

std::vector<uint8_t> latestRawData;
std::vector<uint8_t> previousRawData;

SemaphoreHandle_t bleMutex;
SemaphoreHandle_t bleNotifyMutex;

static constexpr uint32_t scanTimeMs = 5 * 1000;

bool bleInit = true;
bool newDataAvailable = false;

NimBLERemoteService* dataRemoteService = nullptr;
NimBLERemoteCharacteristic* infDisplayDataCharacteristic = nullptr;
NimBLERemoteCharacteristic* clientWriteCharacteristic = nullptr;
NimBLEClient* pClient = nullptr;
NimBLEScan* pBLEScan = nullptr;

NimBLEServer* pServer;
NimBLEService* pRadarService;
NimBLECharacteristic* pAlertNotifyChar;
NimBLECharacteristic* pCommandWriteChar;

const uint8_t notificationOn[] = {0x1, 0x0};

void restartAdvertisingTask(void* param) {
  vTaskDelay(pdMS_TO_TICKS(150));
  NimBLEDevice::startAdvertising();
  vTaskDelete(NULL);
}

void onProxyReady() {
  bleNotifyMutex = xSemaphoreCreateMutex();
  if (!NimBLEDevice::getAdvertising()->isAdvertising()) {
    xTaskCreate(restartAdvertisingTask, "adv_restart", 2048, NULL, 1, NULL);

    if (NimBLEDevice::startAdvertising()) {
      Serial.println("Advertising started after client connection.");
    }
  }
}

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    Serial.printf("BLE Connected to: %s on core %d\n", pClient->getPeerAddress().toString().c_str(), xPortGetCoreID());
    bt_connected = true;
    bleInit = true;
    
    if (settings.proxyBLE) {
      onProxyReady();
    }
  }

  void onDisconnect(NimBLEClient* pClient, int reason) override {
    Serial.printf("%s Disconnected, reason = %d - Restarting scan in 2s\n", 
                  pClient->getPeerAddress().toString().c_str(), reason);

    bt_connected = false;
    if (settings.proxyBLE) {
      NimBLEDevice::stopAdvertising();
    }

    xTaskCreate([](void*){
      vTaskDelay(pdMS_TO_TICKS(2000));
      NimBLEDevice::getScan()->start(scanTimeMs);
      vTaskDelete(NULL);
    }, "BLEScanRestart", 2048, NULL, 1, NULL);
  }

  void onConnectFail(NimBLEClient* pClient, int reason) override {
    Serial.printf("%s Connect failed, reason = %d\n", 
      pClient->getPeerAddress().toString().c_str(), reason);
  }
} clientCallbacks;
  
class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {

    if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(bmeServiceUUID)) {
      //std::string serviceUuid = advertisedDevice->getServiceUUID().toString(); // service uuid
      std::string deviceName = advertisedDevice->getName(); // common name
      std::string deviceAddrStr = advertisedDevice->getAddress().toString(); // mac

      if (deviceName.length() < 3) return;
      std::string devicePrefix = deviceName.substr(0,3);

      bool doBLEConnect;

      if (devicePrefix == "V1C") {
        Serial.printf("%s found: %s | MAC: %s\n", devicePrefix.c_str(), deviceName.c_str(), deviceAddrStr.c_str());
        v1le = true;
        if (settings.useV1LE && !bt_connected) {
          Serial.println("Attempting to connect to V1C LE");
          NimBLEDevice::getScan()->stop();
          doBLEConnect = true;

          pClient = NimBLEDevice::getClientByPeerAddress(advertisedDevice->getAddress());
          if (!pClient) {
            //Serial.println("No disconnected client available, creating new one...");
            pClient = NimBLEDevice::createClient(advertisedDevice->getAddress());
          }
        } else {
          Serial.println("use V1C LE set to false; skipping...");
        }
      }
      else if (devicePrefix == "V1G") {
        Serial.printf("%s found: %s | MAC: %s\n", devicePrefix.c_str(), deviceName.c_str(), deviceAddrStr.c_str());
        if (!settings.useV1LE && !bt_connected) {
          Serial.println("Attempting to connect to V1G");
          NimBLEDevice::getScan()->stop();
          doBLEConnect = true;

          pClient = NimBLEDevice::getClientByPeerAddress(advertisedDevice->getAddress());
          if (!pClient) {
            //Serial.println("No disconnected client available, creating new one...");
            pClient = NimBLEDevice::createClient(advertisedDevice->getAddress());
            if (!pClient) {
              Serial.printf("Failed to create client\n");
              return;
            }
          }
        }
      }

      if (doBLEConnect && pClient) {
        if (!pClient->connect(true, true, false)) {
          Serial.println("Failed to connect, deleting client...");
          NimBLEDevice::deleteClient(pClient);
          pClient = nullptr;
          return;
        }
        pClient->setClientCallbacks(&clientCallbacks, false);
      }
    }
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override {
      bt_connected = false;
      NimBLEDevice::getScan()->start(scanTimeMs);
  }
} scanCallbacks;

class CommandWriteCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string cmd = pCharacteristic->getValue();

    if (clientWriteCharacteristic) {
      if (clientWriteCharacteristic->writeValue(cmd)) {
        /*
        Serial.print("Command forwarded to V1G2: ");
        for (char c : cmd) Serial.printf("%02X ", (uint8_t)c);
        Serial.println();
        */
      } else {
        Serial.println("Failed to forward command.");
      }
    } else {
      Serial.println("Write characteristic not ready.");
    }
  }
};

class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    unsigned long proxyConnect = millis() - bootMillis;
    Serial.printf("BLE client connected to proxy after %.2f seconds\n", proxyConnect / 1000.0);
    proxyConnected = true;
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    proxyConnected = false;
    if (bt_connected) {
      Serial.println("BLE client disconnected, restart advertising");
      NimBLEDevice::startAdvertising();
    }
  }
};

static void notifyDisplayCallbackv2(NimBLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (!pData) return;
  
  std::vector<uint8_t> tempRawData(pData, pData + length);
  tempRawData.assign(pData, pData + length);

  if (tempRawData != previousRawData) {
    previousRawData = tempRawData;
    latestRawData = std::move(tempRawData);
    newDataAvailable = true;
  }

  if (pAlertNotifyChar) {
    if (xSemaphoreTake(bleNotifyMutex, pdMS_TO_TICKS(50))) {
      pAlertNotifyChar->setValue(pData, length);
      pAlertNotifyChar->notify();
      xSemaphoreGive(bleNotifyMutex);
    }
  }
}

void displayReader(NimBLEClient* pClient) {

    dataRemoteService = pClient->getService(bmeServiceUUID);
    if (dataRemoteService) {
      infDisplayDataCharacteristic = dataRemoteService->getCharacteristic(infDisplayDataUUID);
      if (infDisplayDataCharacteristic) {
        if (infDisplayDataCharacteristic->canNotify()) {
          infDisplayDataCharacteristic->subscribe(true, notifyDisplayCallbackv2);
          Serial.println("Subscribed to alert notifications.");
        } else {
          Serial.println("Characteristic does not support notifications.");
        }
      } else {
        Serial.println("Failed to find infDisplayDataCharacteristic.");
      }
  
      infDisplayDataCharacteristic = dataRemoteService->getCharacteristic(infDisplayDataUUID);
      clientWriteCharacteristic = dataRemoteService->getCharacteristic(clientWriteUUID);
      if (infDisplayDataCharacteristic && clientWriteCharacteristic) {
        NimBLERemoteDescriptor* pDesc = infDisplayDataCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
        if (pDesc) {
          pDesc->writeValue((uint8_t*)notificationOn, 2, true);
        }
        delay(50);
        if (settings.turnOffDisplay) {
          uint8_t value = settings.onlyDisplayBTIcon ? 0x01 : 0x00;
          clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqTurnOffMainDisplay(value), 8, false);
          delay(50);
        }
        clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqStartAlertData(), 7, false);
      }
    } else {
      Serial.println("Failed to find bmeServiceUUID!");
    }
}

void queryDeviceInfo(NimBLEClient* pClient) {
    NimBLERemoteService* pService = pClient->getService(deviceInfoUUID);
  
    if (pService == nullptr) {
      Serial.println("Device Information Service not found.");
      return;
    }
  
    struct CharacteristicMapping {
      const char* name;
      const char* uuid;
      std::string* storage;
    } characteristics[] = {
      {"Manufacturer Name", "2A29", &manufacturerName},
      {"Model Number", "2A24", &modelNumber},
      //{"Serial Number", "2A25", &serialNumber},
      //{"Hardware Revision", "2A27", &hardwareRevision},
      //{"Firmware Revision", "2A26", &firmwareRevision},
      //{"Software Revision", "2A28", &softwareRevision}
    };
  
    for (const auto& charInfo : characteristics) {
      NimBLERemoteCharacteristic* pCharacteristic = pService->getCharacteristic(NimBLEUUID(charInfo.uuid));
      if (pCharacteristic != nullptr) {
        *charInfo.storage = pCharacteristic->readValue();
  
        size_t pos = charInfo.storage->find('\0');
        if (pos != std::string::npos) {
          charInfo.storage->erase(pos);
        }
        Serial.printf("%s: %s\n", charInfo.name, charInfo.storage->c_str());
      } else {
        Serial.printf("%s not found.\n", charInfo.name);
      }
    }
  }

void requestSerialNumber() {
      clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqSerialNumber(), 7, false);
      delay(10);
  }
  
void requestVersion() {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqVersion(), 7, false);
    delay(10);
}

void requestVolume() {
    if (xSemaphoreTake(bleMutex, portMAX_DELAY)) {
      clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqCurrentVolume(), 7, false);
      delay(10);
      xSemaphoreGive(bleMutex);
    }
}

void requestUserBytes() {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqUserBytes(), 7, false);
    delay(10);
}

void requestSweepSections() {
  if (bt_connected && clientWriteCharacteristic) {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqSweepSections(), 7, false);
    delay(10);
  }
}

void requestMaxSweepIndex() {
  if (bt_connected && clientWriteCharacteristic) {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqMaxSweepIndex(), 7, false);
    delay(10);
  }
}

void requestAllSweepDefinitions() {
  if (bt_connected && clientWriteCharacteristic) {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqAllSweepDefinitions(), 7, false);
    delay(10);
  }
}

void reqBatteryVoltage() {
  if (bt_connected && clientWriteCharacteristic && !alertPresent) {
    if (xSemaphoreTake(bleMutex, portMAX_DELAY)) {
      clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqBatteryVoltage(), 7, false);
      delay(10);
      xSemaphoreGive(bleMutex);
    }
  }
}

void reqVolume() {
  if (bt_connected && clientWriteCharacteristic && !alertPresent) {
    if (xSemaphoreTake(bleMutex, portMAX_DELAY)) {
      clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqCurrentVolume(), 7, false);
      delay(10);
      xSemaphoreGive(bleMutex);
    }
  }
}

void requestMute() {
  if (!settings.displayTest && clientWriteCharacteristic && bt_connected) {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqMuteOn(), 7, false);
  }
}

void reqMuteOff() {
  if (!settings.displayTest && clientWriteCharacteristic && bt_connected) {
    clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqMuteOff(), 7, false);
  }
}

void initBLE() {
  bleMutex = xSemaphoreCreateMutex();
  if (settings.proxyBLE) {
    Serial.println("initializing as BLE Proxy");
    NimBLEDevice::init("V1 Proxy");
    NimBLEDevice::setDeviceName("V1C-LE-T4S3");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    initBLEServer();
  } else {
    Serial.println("initializing as BLE client only");
    NimBLEDevice::init("V1G2 Client");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  }

  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(&scanCallbacks);
  pScan->setInterval(100);
  pScan->setWindow(75);
  pScan->setActiveScan(true);
  pScan->start(scanTimeMs);
}

void initBLEServer() {
  pServer = NimBLEDevice::createServer();
  pRadarService = pServer->createService(bmeServiceUUID);

  pAlertNotifyChar = pRadarService->createCharacteristic(
    infDisplayDataUUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  pCommandWriteChar = pRadarService->createCharacteristic(
    clientWriteUUID,
    NIMBLE_PROPERTY::WRITE
  );

  pCommandWriteChar->setCallbacks(new CommandWriteCallback());
  pRadarService->start();

  pServer->setCallbacks(new MyServerCallbacks());

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  NimBLEAdvertisementData advData;
  NimBLEAdvertisementData scanRespData;
  
  //advData.setName("V1C-LE-T4S3");
  advData.setCompleteServices(pRadarService->getUUID());
  advData.setAppearance(0x0C80);
  scanRespData.setName("V1C-LE-T4S3");
  
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->setScanResponseData(scanRespData);

  pAdvertising->start();

  if (!bt_connected) {
    NimBLEDevice::stopAdvertising();
  }
}
