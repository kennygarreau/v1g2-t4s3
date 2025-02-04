#include "v1_fs.h"

bool SPIFFSFileManager::init() {
    return SPIFFS.begin();
}

File SPIFFSFileManager::openFile(const char* filePath, const char* mode) {
    return SPIFFS.open(filePath, mode);
}

void SPIFFSFileManager::closeFile(File file) {
    file.close();
}
