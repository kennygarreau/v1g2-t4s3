#include "v1_config.h"
#include "v1_fs.h"
#include <sqlite3.h>

sqlite3 *db;
char *errMsg = 0;
static uint8_t *psramBuffer = NULL;


void listSPIFFSFiles() {
    Serial.println("Listing SPIFFS files...");

    File root = SPIFFS.open("/");
    if (!root) {
        Serial.println("Failed to open SPIFFS root directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        Serial.printf("FILE: %s - %d bytes\n", file.name(), file.size());
        file = root.openNextFile();
    }

    Serial.println("Finished listing files.");
}

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

void SPIFFSFileManager::createTable() {
    const char *sql = "CREATE TABLE lockouts ("
        "id INTEGER PRIMARY KEY, "
        "active INTEGER, "
        "entryType INTEGER, "
        "timestamp INTEGER, "
        "lastSeen INTEGER, "
        "counter INTEGER, "
        "latitude REAL, "
        "longitude REAL, "
        "speed INTEGER, "
        "course INTEGER, "
        "strength INTEGER, "
        "direction INTEGER, "
        "frequency INTEGER"
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

    File file = SPIFFS.open(DB_PATH, FILE_WRITE);
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
    File file = SPIFFS.open("/spiffs/test.txt", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to create test file");
    } else {
        file.println("Test write");
        file.close();
        Serial.println("Test file created successfully");
    }

    if (SPIFFS.exists("/spiffs/test.txt")) {
        if (SPIFFS.remove("/spiffs/test.txt")) {
            Serial.println("Test file removed successfully");
        } else {
            Serial.println("Failed to remove test file");
        }
    }
}