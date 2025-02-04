#ifndef SPIFFS_FILE_MANAGER_H
#define SPIFFS_FILE_MANAGER_H

#include <SPIFFS.h>

class SPIFFSFileManager {
public:
    SPIFFSFileManager() {}

    bool init();
    File openFile(const char* filePath, const char* mode);
    void closeFile(File file);
};

#endif
