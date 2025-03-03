#include "v1_fs.h"
#include "v1_config.h"
#include <ArduinoJson.h>

bool SPIFFSFileManager::init() {
    return SPIFFS.begin();
}

File SPIFFSFileManager::openFile(const char* filePath, const char* mode) {
    return SPIFFS.open(filePath, mode);
}

void SPIFFSFileManager::closeFile(File file) {
    file.close();
}

uint32_t SPIFFSFileManager::getStorageTotal() {
    uint32_t total = SPIFFS.totalBytes();
    uint32_t totalKB = total / 1024;

    return totalKB;
}

uint32_t SPIFFSFileManager::getStorageUsed() {
    uint32_t used = SPIFFS.usedBytes();
    uint32_t usedKB = used / 1024;

    return usedKB;
}

void SPIFFSFileManager::writeLockoutEntryAsJson(const char* filePath, LockoutEntry entry) {
    JsonDocument doc;

    doc["timestamp"] = entry.timestamp;
    doc["latitude"] = entry.latitude;
    doc["longitude"] = entry.longitude;
    //doc["bands"] = entry.bands;

    String jsonString;
    serializeJson(doc, jsonString);

    File file = SPIFFS.open(filePath, "a");

    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    file.println(jsonString);
    file.close();
}

void SPIFFSFileManager::readLockoutEntriesFromJson(const char* filePath) {
    File file = SPIFFS.open(filePath, "r");

    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    while (file.available()) {
        String jsonString = file.readStringUntil('\n');
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonString);

        if (error) {
            Serial.println("Failed to parse JSON");
            continue;
        }

        uint32_t timestamp = doc["timestamp"];
        double latitude = doc["latitude"];
        double longitude = doc["longitude"];

        Serial.print("Timestamp: ");
        Serial.print(timestamp);
        Serial.print(" Latitude: ");
        Serial.print(latitude);
        Serial.print(" Longitude: ");
        Serial.println(longitude);
    }

    file.close();
}

bool SPIFFSFileManager::removeLockoutEntryByTimestamp(const char* filePath, uint32_t timestampToRemove) {
    File file = SPIFFS.open(filePath, "r");

    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }

    File tempFile = SPIFFS.open("/temp_lockouts.dat", "w");

    if (!tempFile) {
        Serial.println("Failed to open temporary file for writing");
        file.close();
        return false;
    }

    bool entryFound = false;
    
    while (file.available()) {
        String jsonString = file.readStringUntil('\n');
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonString);

        if (error) {
            Serial.println("Failed to parse JSON");
            continue;
        }

        uint32_t timestamp = doc["timestamp"];

        if (timestamp == timestampToRemove) {
            entryFound = true;
            continue;
        }

        String newJsonString;
        serializeJson(doc, newJsonString);
        tempFile.println(newJsonString);
    }

    file.close();
    tempFile.close();

    if (!entryFound) {
        Serial.println("Entry with specified timestamp not found");
        SPIFFS.remove("/temp_lockouts.dat");
        return false;
    }

    SPIFFS.remove(filePath);
    SPIFFS.rename("/temp_lockouts.dat", filePath);

    Serial.println("Entry removed successfully");
    return true;
}
