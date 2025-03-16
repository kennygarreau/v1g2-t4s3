#ifndef SPIFFS_FILE_MANAGER_H
#define SPIFFS_FILE_MANAGER_H

#include <SPIFFS.h>
#include "v1_config.h"

#define DB_PATH "/spiffs/lockouts.db"
#define CACHE_SIZE 4

class SPIFFSFileManager {
private:

public:
    SPIFFSFileManager() {}

    bool init();
    File openFile(const char* filePath, const char* mode);
    void closeFile(File file);

    bool openDatabase();
    void closeDatabase();
    void createTable();
    void insertLockoutEntry(const LockoutEntry &entry);
    void updateEntryType(uint32_t id, bool newType);
    void readLockouts();
    void flushToDB(const std::vector<LockoutEntry> &entries);
    void testWrite();

    uint32_t getStorageTotal();
    uint32_t getStorageUsed();
};

#endif
