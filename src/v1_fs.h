#ifndef SPIFFS_FILE_MANAGER_H
#define SPIFFS_FILE_MANAGER_H

#include <SPIFFS.h>
#include "v1_config.h"

class SPIFFSFileManager {
private:

public:

    struct LockoutEntry {
        uint32_t timestamp;
        double latitude;
        double longitude;
    };

    SPIFFSFileManager() {}

    bool init();
    File openFile(const char* filePath, const char* mode);
    void closeFile(File file);

    void writeLockoutEntryAsJson(const char* filePath, LockoutEntry entry);
    void readLockoutEntriesFromJson(const char* filePath);
    bool removeLockoutEntryByTimestamp(const char* filePath, uint32_t timestampToRemove);

    uint32_t getStorageTotal();
    uint32_t getStorageUsed();
};

#endif
