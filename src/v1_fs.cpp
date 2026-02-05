#include "v1_config.h"
#include "v1_fs.h"
#include <sqlite3.h>

#include "FS.h"
#include "LittleFS.h"
#include <ArduinoJson.h>

constexpr size_t MAX_LOG_FILES = 30; // Keep the last 30 "days"
constexpr size_t MAX_FILE_SIZE = 100 * 1024; // 100KB

sqlite3 *db;
char *errMsg = 0;
static uint8_t *psramBuffer = NULL;

bool initStorage() {
    if (!LittleFS.begin(true, "/littlefs", 10, "littlefs")) { 
        Serial.println("LittleFS mount failed");
        return false;
    }

    Serial.printf("LittleFS: Total=%d KB, Used=%d KB\n",
        LittleFS.totalBytes() / 1024, LittleFS.usedBytes() / 1024);
    ensureLogDir();
    return true;
}

String getLogFilename(uint32_t timestamp) {
    uint32_t daysSinceEpoch = timestamp / 86400;
    
    // Build path more explicitly
    String path = "/logs/";
    path += String(daysSinceEpoch);
    path += ".jsonl";
    
    //Serial.printf("[DEBUG] Generated filename: %s\n", path.c_str());
    
    return path;
}

void ensureLogDir() {
    if (!LittleFS.exists("/logs")) {
        LittleFS.mkdir("/logs");
    }
}

void pruneOldLogFiles() {
    File root = LittleFS.open("/logs");
    if (!root || !root.isDirectory()) return;

    std::vector<String> files;
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".jsonl")) {
            files.push_back(String(file.name()));
        }
        file = root.openNextFile();
    }

    std::sort(files.begin(), files.end());

    while (files.size() > MAX_LOG_FILES) {
        String toDelete = files.front();
        Serial.printf("Deleting old log file: %s\n", toDelete.c_str());
        LittleFS.remove(toDelete);
        files.erase(files.begin());
    }
    Serial.printf("Log files retained: %d\n", files.size());
}

std::vector<String> getLogFileList() {
    std::vector<String> files;
    File root = LittleFS.open("/logs");
    if (!root || !root.isDirectory()) return files;

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String fileName = String(file.name());
            
            if (!fileName.startsWith("/logs/")) {
                fileName = "/logs/" + fileName;
            }

            if (fileName.endsWith(".jsonl")) {
                files.push_back(fileName);
            }
        }
        file = root.openNextFile();
    }

    std::sort(files.rbegin(), files.rend());
    return files;
}

bool SPIFFSFileManager::init() {
    return LittleFS.begin();
}

File SPIFFSFileManager::openFile(const char* filePath, const char* mode) {
    return LittleFS.open(filePath, mode);
}

void SPIFFSFileManager::closeFile(File file) {
    file.close();
}

uint32_t SPIFFSFileManager::getStorageTotal() {
    uint32_t total = LittleFS.totalBytes();
    uint32_t totalKB = total / 1024;

    return totalKB;
}

uint32_t SPIFFSFileManager::getStorageUsed() {
    uint32_t used = LittleFS.usedBytes();
    uint32_t usedKB = used / 1024;

    return usedKB;
}

void SPIFFSFileManager::createTable() {
    const char *sql = "CREATE TABLE IF NOT EXISTS lockouts ("
        "id INTEGER PRIMARY KEY, "
        "active INTEGER NOT NULL, "
        "entryType INTEGER NOT NULL, "
        "timestamp INTEGER NOT NULL, "
        "lastSeen INTEGER NOT NULL, "
        "counter INTEGER NOT NULL, "
        "latitude REAL NOT NULL, "
        "longitude REAL NOT NULL, "
        "speed INTEGER NOT NULL, "
        "course INTEGER NOT NULL, "
        "strength INTEGER NOT NULL, "
        "direction INTEGER NOT NULL, "
        "frequency INTEGER NOT NULL"
        ");";
    
    char *errMsg;
    if (sqlite3_exec(db, sql, NULL, NULL, &errMsg) != SQLITE_OK) {
        Serial.printf("Create table failed: %s\n", errMsg);
        sqlite3_free(errMsg);
        
    } else {
        Serial.println("Table created or already exists.");
    }
}

void SPIFFSFileManager::insertLockoutEntry(const LockoutEntry &entry) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO lockouts ("
                      "active, entryType, timestamp, lastSeen, counter, "
                      "latitude, longitude, speed, course, strength, "
                      "direction, frequency) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        Serial.printf("SQL prepare failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int(stmt, 1, entry.active);
    sqlite3_bind_int(stmt, 2, entry.entryType);
    sqlite3_bind_int(stmt, 3, entry.timestamp);
    sqlite3_bind_int(stmt, 4, entry.lastSeen);
    sqlite3_bind_int(stmt, 5, entry.counter);
    sqlite3_bind_double(stmt, 6, entry.latitude);
    sqlite3_bind_double(stmt, 7, entry.longitude);
    sqlite3_bind_int(stmt, 8, entry.speed);
    sqlite3_bind_int(stmt, 9, entry.course);
    sqlite3_bind_int(stmt, 10, entry.strength);
    sqlite3_bind_int(stmt, 11, entry.direction);
    sqlite3_bind_int(stmt, 12, entry.frequency);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Serial.printf("Insert failed: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}

void SPIFFSFileManager::updateEntryType(uint32_t id, bool newType) {
    char sql[128];
    snprintf(sql, sizeof(sql), "UPDATE lockouts SET entryType=%d WHERE id=%u;", newType, id);

    if (sqlite3_exec(db, sql, NULL, NULL, &errMsg) != SQLITE_OK) {
        Serial.printf("Update failed: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
}

void SPIFFSFileManager::readLockouts() {
    const char *sql = "SELECT active, entryType, timestamp, lastSeen, counter, latitude, longitude, "
                      "speed, course, strength, direction, frequency FROM lockouts;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        Serial.printf("SQL prepare failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    Serial.println("---- Lockout Entries ----");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LockoutEntry entry;
        entry.active = sqlite3_column_int(stmt, 0);
        entry.entryType = sqlite3_column_int(stmt, 1);
        entry.timestamp = sqlite3_column_int(stmt, 2);
        entry.lastSeen = sqlite3_column_int(stmt, 3);
        entry.counter = sqlite3_column_int(stmt, 4);
        entry.latitude = sqlite3_column_double(stmt, 5);
        entry.longitude = sqlite3_column_double(stmt, 6);
        entry.speed = sqlite3_column_int(stmt, 7);
        entry.course = sqlite3_column_int(stmt, 8);
        entry.strength = sqlite3_column_int(stmt, 9);
        entry.direction = sqlite3_column_int(stmt, 10);
        entry.frequency = sqlite3_column_int(stmt, 11);

        const char *directionStr = entry.direction ? "Rear" : "Front";

        Serial.printf("Active: %d | Type: %s | Timestamp: %u | Last Seen: %u | Counter: %d | "
                      "Lat: %.8f | Lon: %.8f | Speed: %d | Course: %d | Strength: %d | "
                      "Direction: %s | Freq: %d\n",
                      entry.active, entry.entryType ? "Manual" : "Auto",
                      entry.timestamp, entry.lastSeen, entry.counter,
                      entry.latitude, entry.longitude,
                      entry.speed, entry.course, entry.strength,
                      directionStr, entry.frequency);
    }

    sqlite3_finalize(stmt);
}

bool SPIFFSFileManager::openDatabase() {
    if (!psramBuffer) {
        psramBuffer = (uint8_t *)heap_caps_malloc(SQLITE_PSRAM_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
        if (!psramBuffer) {
            Serial.println("Failed to allocate PSRAM for SQLite");
            return false;
        }
        sqlite3_config(SQLITE_CONFIG_HEAP, psramBuffer, SQLITE_PSRAM_BUFFER_SIZE, 64);
        Serial.println("SQLite memory allocated in PSRAM");
    }
    //listSPIFFSFiles();

    File file = LittleFS.open(DB_PATH, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to create new database file");
        return false;
    }
    file.close();
    Serial.println("New database file created");

    int rc = sqlite3_open(DB_PATH, &db);
    if (rc != SQLITE_OK) {
        Serial.printf("Failed to open database: %s\n", sqlite3_errmsg(db));
        return false;
    }    

    Serial.println("Setting PRAGMA synchronous");
    sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, &errMsg);
    if (errMsg) {
        Serial.printf("PRAGMA synchronous failed: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    Serial.println("Setting PRAGMA journal_mode");
    sqlite3_exec(db, "PRAGMA journal_mode = MEMORY;", NULL, NULL, &errMsg);
    if (errMsg) {
        Serial.printf("PRAGMA journal_mode failed: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    Serial.println("Setting PRAGMA temp_store");
    sqlite3_exec(db, "PRAGMA temp_store = MEMORY;", NULL, NULL, &errMsg);
    if (errMsg) {
        Serial.printf("PRAGMA temp_store failed: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    Serial.println("Database opened successfully");
    return true;
} 

void SPIFFSFileManager::closeDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
        Serial.println("Database closed.");
    }
}

void SPIFFSFileManager::flushToDB(const std::vector<LockoutEntry> &entries) {
    if (!openDatabase()) return;

    for (const auto &entry : entries) {
        insertLockoutEntry(entry);
    }

    closeDatabase();
}

void SPIFFSFileManager::testWrite() {
    File file = LittleFS.open("/spiffs/test.txt", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to create test file");
    } else {
        file.println("Test write");
        file.close();
        Serial.println("Test file created successfully");
    }

    if (LittleFS.exists("/spiffs/test.txt")) {
        if (LittleFS.remove("/spiffs/test.txt")) {
            Serial.println("Test file removed successfully");
        } else {
            Serial.println("Failed to remove test file");
        }
    }
}