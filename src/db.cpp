#include <sqlite3.h>
#include "v1_packet.h"

void commitLogToDB(sqlite3* db, const std::vector<LogEntry>& logHistory) {
    for (const auto& entry : logHistory) {
        double latDelta = 0.5 / 69.0;
        double lonDelta = 0.5 / (69.0 * cos(entry.latitude * M_PI / 180.0));

        double minLat = entry.latitude - latDelta;
        double maxLat = entry.latitude + latDelta;
        double minLon = entry.longitude - lonDelta;
        double maxLon = entry.longitude + lonDelta;

        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db,
            "SELECT 1 FROM alerts WHERE frequency = ? AND latitude BETWEEN ? AND ? AND longitude BETWEEN ? AND ? LIMIT 1",
            -1, &stmt, nullptr);
        
        sqlite3_bind_int(stmt, 1, entry.frequency);
        sqlite3_bind_double(stmt, 2, minLat);
        sqlite3_bind_double(stmt, 3, maxLat);
        sqlite3_bind_double(stmt, 4, minLon);
        sqlite3_bind_double(stmt, 5, maxLon);

        bool shouldInsert = sqlite3_step(stmt) != SQLITE_ROW;
        sqlite3_finalize(stmt);

        if (shouldInsert) {
            sqlite3_prepare_v2(db, "INSERT INTO alerts (timestamp, latitude, longitude, speed, course, strength, direction, frequency) VALUES (?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, static_cast<int>(entry.timestamp));
            sqlite3_bind_double(stmt, 2, entry.latitude);
            sqlite3_bind_double(stmt, 3, entry.longitude);
            sqlite3_bind_double(stmt, 4, entry.speed);
            sqlite3_bind_int(stmt, 5, entry.course);
            sqlite3_bind_int(stmt, 6, entry.strength);
            sqlite3_bind_int(stmt, 7, entry.direction);
            sqlite3_bind_int(stmt, 8, entry.frequency);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
}