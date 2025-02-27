#ifndef SPIFFS_FILE_MANAGER_H
#define SPIFFS_FILE_MANAGER_H

#include <SPIFFS.h>

class SPIFFSFileManager {
private:
    struct LockoutEntry {
        uint32_t timestamp;
        double latitude;
        double longitude;
    };
public:

    SPIFFSFileManager() {}

    bool init();
    File openFile(const char* filePath, const char* mode);
    void closeFile(File file);
    uint32_t getStorageTotal();
    uint32_t getStorageUsed();

    void writeLockoutEntry(const char* filePath, LockoutEntry entry) {
        File file = openFile(filePath, "a");

        if (!file) {
            Serial.println("Failed to open file for writing");
            return;
        }

        // Write the timestamp, latitude, and longitude to the file
        file.write((uint8_t*)&entry.timestamp, sizeof(entry.timestamp));
        file.write((uint8_t*)&entry.latitude, sizeof(entry.latitude));
        file.write((uint8_t*)&entry.longitude, sizeof(entry.longitude));

        closeFile(file);
    }

    void readLockoutEntries(const char* filePath) {
        File file = openFile(filePath, "r");

        if (!file) {
            Serial.println("Failed to open file for reading");
            return;
        }

        LockoutEntry entry;

        while (file.available()) {
            file.read((uint8_t*)&entry.timestamp, sizeof(entry.timestamp));
            file.read((uint8_t*)&entry.latitude, sizeof(entry.latitude));
            file.read((uint8_t*)&entry.longitude, sizeof(entry.longitude));

            Serial.print("Timestamp: ");
            Serial.print(entry.timestamp);
            Serial.print(" Latitude: ");
            Serial.print(entry.latitude, 6);
            Serial.print(" Longitude: ");
            Serial.println(entry.longitude, 6);
        }

        closeFile(file);
    }
};

#endif
