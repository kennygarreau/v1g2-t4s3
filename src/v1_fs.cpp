#include "v1_fs.h"
#include "v1_config.h"

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