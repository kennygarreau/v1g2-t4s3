#include "ble.h"
#include "v1_packet.h"
#include "v1_config.h"
#include "utils.h"
#include "ui/ui.h"
#include "ui/actions.h"
#include "ui/blinking.h"
#include <set>

std::vector<uint8_t> lastRawInfPayload;
bool priority, junkAlert, alertPresent, muted, remoteAudio, savvy;
static uint8_t alertCountValue, alertIndexValue;
std::string prio_alert_freq = "";
static char current_alerts[MAX_ALERTS][32];
uint8_t activeBands = 0;
uint8_t lastReceivedBands = 0;

std::vector<LogEntry> logHistory;
alertsVectorRaw alertTableRaw;
Config globalConfig;

static bool k_rcvd = false;
static bool ka_rcvd = false;
static bool zero_rcvd = false;

BandState ka_state = {false, 0};
BandState k_state = {false, 0};
BandState x_state = {false, 0};
BandState laser_state = {false, 0};

BandState front_state = {false, 0};
BandState side_state = {false, 0};
BandState rear_state = {false, 0};

extern void requestMute();
uint8_t packet[10];

PacketDecoder::PacketDecoder(const std::vector<uint8_t>& rawpacket)
  : rawpacket(rawpacket) {}

int combineMSBLSB_v2(uint8_t msb, uint8_t lsb) {
    return (static_cast<int>(msb) << 8) | lsb;
}

char byteToAscii(uint8_t byte) {
    return std::isprint(byte) ? byte : '.';
}

void updateActiveBands(uint8_t band) {
    activeBands |= band;
}

void clearInactiveBands(uint8_t newBandData) {
    activeBands &= (lastReceivedBands | newBandData); 
}

uint8_t mapXToBars(uint8_t& value) {
    static constexpr uint8_t thresholds[] = {0x00, 0x95, 0x9F, 0xA9, 0xB3, 0xBC, 0xC4, 0xCF, 0xFF};

    for (int i = 0; i < 8; ++i) {
        if (value <= thresholds[i]) return i;
    }

    return -1;
}

uint8_t mapKToBars(uint8_t& value) {
    static constexpr uint8_t thresholds[] = {0x00, 0x87, 0x8F, 0x99, 0xA3, 0xAD, 0xB7, 0xC1, 0xFF};

    for (int i = 0; i < 8; ++i) {
        if (value <= thresholds[i]) return i;
    }

    return -1;
}

uint8_t mapKaToBars(uint8_t& value) {
    static constexpr uint8_t thresholds[] = {0x00, 0x8F, 0x96, 0x9D, 0xA4, 0xAB, 0xB2, 0xB9, 0xFF};

    for (int i = 0; i < 8; ++i) {
        if (value <= thresholds[i]) return i;
    }

    return -1;
}

void processSection_v2(std::vector<uint8_t> packet, uint8_t offset) {
    uint8_t sweepDefIndexNum = packet[offset + 5];
    uint8_t sectionCount = sweepDefIndexNum & 0b00001111;
    uint8_t sectionIndex = (sweepDefIndexNum & 0b11110000) >> 4;

    int upperBound = combineMSBLSB_v2(packet[sweepDefIndexNum + 1], packet[sweepDefIndexNum + 2]);
    int lowerBound = combineMSBLSB_v2(packet[sweepDefIndexNum + 3], packet[sweepDefIndexNum + 4]);
    
    Serial.printf("section %d: lower edge: %d, upper edge: %d\n", sectionIndex, lowerBound, upperBound);
    globalConfig.sections.emplace_back(lowerBound, upperBound);
}

BandArrowData processBandArrow_v2(uint8_t& bandArrow) {
    BandArrowData data = {false};

    data.laser = (bandArrow & 0b00000001) != 0;
    data.ka = (bandArrow & 0b00000010) != 0;
    data.k = (bandArrow & 0b00000100) != 0;
    data.x = (bandArrow & 0b00001000) != 0;
    data.muteIndicator = (bandArrow & 0b00010000) != 0;
    data.front = (bandArrow & 0b00100000) != 0;
    data.side = (bandArrow & 0b01000000) != 0;
    data.rear = (bandArrow & 0b10000000) != 0;

//    clearInactiveBands(bandArrow);

    return data;
}

void updateBandActivity(bool ka, bool k, bool x, bool laser) {
    uint32_t now = millis();

    //Serial.printf("updateBandActivity: ka=%d k=%d x=%d laser=%d, now=%u\n", ka, k, x, laser, now);

    if (ka) {
        ka_state.last_seen_ms = now;
        ka_state.active = true;
    }
    
    if (k) {
        k_state.last_seen_ms = now;
        k_state.active = true;
    }
    
    if (x) {
        x_state.last_seen_ms = now;
        x_state.active = true;
    }
    
    if (laser) {
        laser_state.last_seen_ms = now;
        laser_state.active = true;
    }
}

void updateArrowActivity(bool front, bool side, bool rear) {
    uint32_t now = millis();

    if (front) {
        front_state.active = true;
        front_state.last_seen_ms = now;
    }
    if (side) {
        side_state.active = true;
        side_state.last_seen_ms = now;
    }
    if (rear) {
        rear_state.active = true;
        rear_state.last_seen_ms = now;
    }
}

void checkBandTimeouts() {
    uint32_t now = millis();

    if (ka_state.active && (now - ka_state.last_seen_ms > BAND_TIMEOUT_MS)) {
        //Serial.printf("deactivating Ka band, last seen delta: %u\n", now - ka_state.last_seen_ms);
        ka_state.active = false;
        disable_blinking(BLINK_KA);
    }
    if (k_state.active && (now - k_state.last_seen_ms > BAND_TIMEOUT_MS)) {
        k_state.active = false;
        disable_blinking(BLINK_K);
    }
    if (x_state.active && (now - x_state.last_seen_ms > BAND_TIMEOUT_MS)) {
        x_state.active = false;
        disable_blinking(BLINK_X);
    }
    if (laser_state.active && (now - laser_state.last_seen_ms > BAND_TIMEOUT_MS)) {
        laser_state.active = false;
        disable_blinking(BLINK_LASER);
    }
    if (front_state.active && (now - front_state.last_seen_ms > BAND_TIMEOUT_MS)) {
        front_state.active = false;
        disable_blinking(BLINK_FRONT);
    }
    if (side_state.active && (now - side_state.last_seen_ms > BAND_TIMEOUT_MS)) {
        side_state.active = false;
        disable_blinking(BLINK_SIDE);
    }
    if (rear_state.active && (now - rear_state.last_seen_ms > BAND_TIMEOUT_MS)) {
        rear_state.active = false;
        disable_blinking(BLINK_REAR);
    }
}

void compareBandArrows(const BandArrowData& arrow1, const BandArrowData& arrow2) {
    //Serial.printf("* compareBandArrows: Ka=%d K=%d X=%d\n", arrow1.ka, arrow1.k, arrow1.x);

    updateBandActivity(arrow1.ka, arrow1.k, arrow1.x, arrow1.laser);
    updateArrowActivity(arrow1.front, arrow1.side, arrow1.rear);
    
    set_var_muted(arrow1.muteIndicator);

    bool anyBandActive = false;

    // Update band blinking
    if (arrow1.ka != arrow2.ka && arrow1.ka) {
        enable_blinking(BLINK_KA);
    }
    
    if (arrow1.k != arrow2.k && arrow1.k) {
        enable_blinking(BLINK_K);
    }
    
    if (arrow1.x != arrow2.x && arrow1.x) {
        enable_blinking(BLINK_X);
    }
    
    if (arrow1.laser != arrow2.laser && arrow1.laser) {
        enable_blinking(BLINK_LASER);
    }

    // Update arrow blinking
    if (arrow1.front != arrow2.front && arrow1.front) {
        enable_blinking(BLINK_FRONT);
    }
    
    if (arrow1.side != arrow2.side && arrow1.side) {
        enable_blinking(BLINK_SIDE);
    }
    
    if (arrow1.rear != arrow2.rear && arrow1.rear) {
        enable_blinking(BLINK_REAR);
    }

    // Update activeBands bitmask
    activeBands = 0;
    if (laser_state.active) {
        activeBands |= 0b00000001;
        anyBandActive = true;
    }
    if (ka_state.active) {
        activeBands |= 0b00000010;
        anyBandActive = true;
    }
    if (k_state.active) {
        activeBands |= 0b00000100;
        anyBandActive = true;
    }
    if (x_state.active) {
        activeBands |= 0b00001000;
        anyBandActive = true;
    }

    if (laser_state.active) {
        if (muted) {
            set_var_muted(false);
            reqMuteOff();
        }
        set_var_prio_alert_freq("LASER");
        set_var_prioBars(7);

        if (front_state.active || rear_state.active) {
            enable_blinking(front_state.active ? BLINK_FRONT : BLINK_REAR);
        }
    }

    if (!anyBandActive) {
        start_clear_inactive_bands_timer();
    }
}

/* 
Execute if we successfully write reqStartAlertData to clientWriteUUID
*/
void PacketDecoder::decodeAlertData_v2(const alertsVectorRaw& alerts, int lowSpeedThreshold, uint8_t currentSpeed) {
    unsigned long startTimeMicros = micros();
    
    //std::string dirValue, bandValue;
    const char* dirValue = nullptr;   // Changed from std::string
    const char* bandValue = nullptr;
    int frontStrengthVal = 0;
    int rearStrengthVal = 0;
    uint16_t freqMhz = 0;
    float freqGhz;
    Direction dir;
    Band bnd;

    std::vector<AlertToLog> alertsToLog;
    alertsToLog.reserve(alerts.size());

    std::vector<AlertTableData> alertDataList;
    AlertTableData newAlertData = {alertCountValue, {}, {}, 0, 0};
    
    for (int i = 0; i < alerts.size(); i++) {
        uint8_t alertIndex = alerts[i][0];
        alertCountValue = alertIndex & 0b00001111;
        alertIndexValue = (alertIndex & 0b11110000) >> 4;

        uint8_t frontStrength = alerts[i][3];
        uint8_t rearStrength = alerts[i][4];
        uint8_t bandArrow = alerts[i][5];
        uint8_t auxByte = alerts[i][6];

        priority = (auxByte & 0b10000000) != 0;
        junkAlert = (auxByte & 0b01000000) != 0;

        uint8_t radarTypeValue = auxByte & 0b00001111;

        const char* radarTypeName =
            (radarTypeValue < 7)
                ? PhotoRadarTypeNames[radarTypeValue]
                : "Unknown";

        if (strlen(radarTypeName) > 1) {
            set_var_photoType(radarTypeName);
            set_var_photoAlertPresent(true);
        }

        switch (bandArrow & 0b00011111) { // Mask the band bits
            case 0b00000001: bandValue = "LASER"; bnd = BAND_LASER; break;
            case 0b00000010: bandValue = "Ka"; bnd = BAND_KA; break;
            case 0b00000100: bandValue = "K"; bnd = BAND_K; break;
            case 0b00001000: bandValue = "X"; bnd = BAND_X; break;
            case 0b00010000: bandValue = "Ku"; bnd = BAND_KU; break;
        }
    
        switch (bandArrow & 0b11100000) { // Mask the direction bits
            case 0b00100000: dirValue = "FRONT"; dir = DIR_FRONT; break;
            case 0b01000000: dirValue = "SIDE"; dir = DIR_SIDE; break;
            case 0b10000000: dirValue = "REAR"; dir = DIR_REAR; break;
        }

        if (bnd == BAND_X && globalConfig.xBand) {
            frontStrengthVal = mapXToBars(frontStrength);
            rearStrengthVal = mapXToBars(rearStrength);
        } 
        else if (bnd == BAND_K || bnd == BAND_KU) {
            frontStrengthVal = mapKToBars(frontStrength);
            rearStrengthVal = mapKToBars(rearStrength);
        }
        else if (bnd == BAND_KA) {
            frontStrengthVal = mapKaToBars(frontStrength);
            rearStrengthVal = mapKaToBars(rearStrength);        
        }
        
        if (bnd != BAND_LASER) {
            uint8_t freqMSB = alerts[i][1];
            uint8_t freqLSB = alerts[i][2];
    
            freqMhz = combineMSBLSB_v2(freqMSB, freqLSB);
            freqGhz = static_cast<float>(freqMhz) / 1000.0f;
        }

        if (priority && bnd != BAND_LASER) {
            if (!muted && gpsAvailable && currentSpeed <= lowSpeedThreshold) {
                requestMute();
                muted = true;
            }

            // paint the main signal bar
            if (rearStrengthVal > frontStrengthVal) {
                set_var_prioBars(rearStrengthVal);
            }
            else {
                set_var_prioBars(frontStrengthVal);
            }

            // paint the frequency of the prio alert
            if (freqGhz > 0) {
                char freqStr[16];
                snprintf(freqStr, sizeof(freqStr), "%.3f", freqGhz);
                set_var_prio_alert_freq(freqStr); 
            }
        }

        if (alertCountValue > 1) {
            bool found = false;
            for (auto& alertData : alertDataList) {
                if (alertData.alertCount == alertCountValue) {
                    alertData.frequencies[alertData.freqCount] = freqGhz;
                    alertData.direction[alertData.freqCount] = dirValue;
                    alertData.freqCount++;
                    found = true;
                    break;
                }
            }
            
            if (!found && !priority) {
                AlertTableData newAlertData = {alertCountValue, {}, {}, 0, 0};
                newAlertData.frequencies[newAlertData.freqCount] = freqGhz;
                newAlertData.direction[newAlertData.freqCount] = dirValue;
                newAlertData.freqCount++;
                alertDataList.push_back(newAlertData);
            }
        }

        unsigned long elapsedTimeMicros = micros() - startTimeMicros;
        if (freqGhz > 0 || bnd == BAND_LASER) {
            uint8_t strength = std::max(frontStrengthVal, rearStrengthVal);
            if (bnd == BAND_LASER) { freqMhz = 3012; strength = 6; }

            if (gpsAvailable && strength >= autoLockoutSettings.minThreshold) {
                alertsToLog.push_back({freqMhz, strength, dir});
            }
        }
    }

    if (!alertsToLog.empty() && xSemaphoreTake(gpsDataMutex, pdMS_TO_TICKS(50))) {
        const uint32_t now = gpsData.rawTime;
        const uint32_t timeWindow = 10;
        
        for (const auto& alert : alertsToLog) {
            auto it = std::find_if(logHistory.begin(), logHistory.end(), 
                [&alert, now, timeWindow](const LogEntry& entry) {
                    return entry.frequency == alert.freqMhz && 
                        (now - entry.timestamp) <= timeWindow;
                });
            
            if (it != logHistory.end()) {
                // Update existing
                if (alert.strength >= it->strength) {
                    it->strength = alert.strength;
                    it->latitude = gpsData.latitude;
                    it->longitude = gpsData.longitude;
                    it->timestamp = now;
                }
            } else {
                // Add new
                LogEntry newEntry = {
                    now, 
                    gpsData.latitude, 
                    gpsData.longitude, 
                    alert.freqMhz,
                    static_cast<uint8_t>(gpsData.course), 
                    gpsData.speed,
                    alert.strength, 
                    alert.dir
                };
                logHistory.push_back(newEntry);
            }
        }
        
        xSemaphoreGive(gpsDataMutex);
    }
/*
            if (gpsAvailable && strength >= autoLockoutSettings.minThreshold && xSemaphoreTake(gpsDataMutex, portMAX_DELAY)) {    
                const uint32_t now = gpsData.rawTime;
                const uint32_t timeWindow = 10;
                
                auto it = std::find_if(logHistory.begin(), logHistory.end(), [freqMhz, now, timeWindow](const LogEntry& entry) {
                    return entry.frequency == freqMhz && (now - entry.timestamp) <= timeWindow;});
                
                if (it != logHistory.end()) {
                    if (strength >= it->strength) {
                        it->strength = strength;
                        it->latitude = gpsData.latitude;
                        it->longitude = gpsData.longitude;
                        it->timestamp = gpsData.rawTime;
                        //Serial.printf("Update existing alert: %u | lat: %f | lon: %f | str: %d | freq: %d | decode(us): %u | history_size: %d\n", now, gpsData.latitude, gpsData.longitude,
                        //            strength, freqMhz, elapsedTimeMicros, logHistory.size());
                    }
                } else {
                    LogEntry newEntry = {gpsData.rawTime, gpsData.latitude, gpsData.longitude, freqMhz, static_cast<u8_t>(gpsData.course), gpsData.speed,
                                        strength, dir};
                    logHistory.push_back(newEntry);
                    // Serial.printf("Logging alert: %u | lat: %f | lon: %f | speed: %d | course: %d | str: %d | dir: %d | freq: %d | decode(us): %u | history_size: %d\n", 
                    //                newEntry.timestamp, newEntry.latitude, newEntry.longitude, newEntry.speed, newEntry.course, 
                    //                newEntry.strength, newEntry.direction, newEntry.frequency, elapsedTimeMicros, logHistory.size());
                }
                xSemaphoreGive(gpsDataMutex);
            } else {
                LogEntry newEntry = {gpsData.rawTime, gpsData.latitude, gpsData.longitude, freqMhz, static_cast<u8_t>(gpsData.course), gpsData.speed,
                    strength, dir};
                //Serial.printf("Unlogged alert: %u | lat: %f | lon: %f | speed: %d | course: %d | str: %d | dir: %d | freq: %d | decode(us): %u | history_size: %d\n", 
                //                    newEntry.timestamp, newEntry.latitude, newEntry.longitude, newEntry.speed, newEntry.course, 
                //                    newEntry.strength, newEntry.direction, newEntry.frequency, elapsedTimeMicros, logHistory.size());
            }
        }
    }
*/

    if (alertCountValue > 1) {

        for (auto& alertData : alertDataList) {
            std::vector<float> uniqueFrequencies;
            std::vector<std::string> uniqueDirections;
        
            for (int i = 0; i < alertData.freqCount; i++) {
                bool freqExists = false;
                for (auto& uniqueFreq : uniqueFrequencies) {
                    if (alertData.frequencies[i] == uniqueFreq) {
                        freqExists = true;
                        break;
                    }
                }
        
                if (!freqExists) {
                    uniqueFrequencies.push_back(alertData.frequencies[i]);
                    uniqueDirections.push_back(alertData.direction[i]);
                }
            }
        
            alertData.freqCount = uniqueFrequencies.size();
            for (int i = 0; i < alertData.freqCount; i++) {
                alertData.frequencies[i] = uniqueFrequencies[i];
                alertData.direction[i] = uniqueDirections[i];
            }
        }
    }

    set_var_alertCount(alertCountValue); // sets the bogey counter

    int tableSize = alertCountValue - 1;
    if (tableSize > MAX_ALERTS) { tableSize = MAX_ALERTS; }
    
    set_var_alertTableSize(tableSize); // should be no larger than 4
    if (tableSize > 0) {
        set_var_showAlertTable(true);
        set_var_frequencies(alertDataList);
    } else {
        set_var_showAlertTable(false);
    }
}

/*  decode operation, passed from loop() - based on the packet ID.
    ID 31 (infDisplayData): route to decodeDisplayData
    ID 43 (respAlertData): direct decode 
*/
std::string PacketDecoder::decode_v2(int lowSpeedThreshold, uint8_t currentSpeed) {
    if (rawpacket.size() > 1 && rawpacket[0] != 0xAA) {
        return "err SOF";
    }

    if (rawpacket.size() > 1 && rawpacket[rawpacket.size() - 1] != 0xAB) {
        return "err EOF";
    }

    uint8_t packetID = rawpacket[3];

    if (packetID == 0x31) {
        //std::vector<uint8_t> payload(rawpacket.begin() + 5, rawpacket.begin() + 13);
        
        //if (settings.displayTest || payload != lastRawInfPayload) {
            uint8_t bandArrow1 = rawpacket[8];
            uint8_t bandArrow2 = rawpacket[9];
            uint8_t aux0 = rawpacket[10];
            uint8_t aux1 = rawpacket[11];
            uint8_t aux2 = rawpacket[12];

            BandArrowData arrow1Data = processBandArrow_v2(bandArrow1);
            BandArrowData arrow2Data = processBandArrow_v2(bandArrow2);

            compareBandArrows(arrow1Data, arrow2Data);

            bool softMute = (aux0 & 0b00000001) ? 1 : 0;
            uint8_t mutedReason = (aux1 & 0b00010000) ? 1 : 0;

            /*
            uint8_t modeBit0 = (aux1 & 0b00000100) ? 1 : 0;
            uint8_t modeBit1 = (aux1 & 0b00001000) ? 2 : 0;
            uint8_t mode = modeBit0 + modeBit1;
            */

            uint8_t mode = ((aux1 >> 2) & 0x03);
            static uint8_t lastMode = 0xFF;

            if (mode != lastMode) {
                globalConfig.rawMode     = mode;
                globalConfig.mode        = modeTable[mode].mode;
                globalConfig.defaultMode = modeTable[mode].defaultMode;
                lastMode = mode;
            }
        //}
    } 
    else if (packetID == 0x43) {
        //std::vector<uint8_t> payload(rawpacket.begin() + 5, rawpacket.begin() + 12);
        uint8_t alertC = rawpacket[5];
        if (alertC == 0x00) {
            if (alertPresent) {
                alertPresent = false;
                photoAlertPresent = false;
            }
            return "";
        }
        else {
            alertCountValue = alertC & 0b00001111;
            alertIndexValue = (alertC & 0b11110000) >> 4;

            std::vector<uint8_t> payload(rawpacket.begin() + 5, rawpacket.begin() + 12);
            alertTableRaw.push_back(payload);

            // check if the alertTable vector size is more than or equal to the tableSize (alerts.count) extracted from alertByte
            if (alertTableRaw.size() >= alertCountValue || alertTableRaw.size() == MAX_ALERTS + 1) {
                alertPresent = true;
                decodeAlertData_v2(alertTableRaw, lowSpeedThreshold, currentSpeed);
                alertTableRaw.clear();
            } 
        }
    }
    else if (packetID == 0x02) {
        //std::vector<uint8_t> payload(rawpacket.begin() + 5, rawpacket.begin() + 12);

        char versionID        = byteToAscii(rawpacket[5]);
        char majorVersion     = byteToAscii(rawpacket[6]);
        char minorVersion     = byteToAscii(rawpacket[8]);
        char revisionDigitOne = byteToAscii(rawpacket[9]);
        char revisionDigitTwo = byteToAscii(rawpacket[10]);
        char controlNumber    = byteToAscii(rawpacket[11]);

        if (versionID == 'V') {
            char versionString[8];
            snprintf(versionString, sizeof(versionString), "%c.%c%c%c%c",
                     majorVersion, minorVersion, revisionDigitOne,
                     revisionDigitTwo, controlNumber);
            softwareRevision = versionString;
            Serial.printf("Software Version: %s\n", versionString);
            versionReceived = true;
        } else if (versionID == 'R') {
            remoteAudio = true;
        } else if (versionID == 'S') {
            savvy = true;
        } else {
            //Serial.printf("Found component: %s", versionID);
        }
    }
    else if (packetID == 0x04) {
        std::string serialString;
        serialString.reserve(10);

        for (uint8_t i = 0; i < 10; i++) {
            serialString += byteToAscii(rawpacket[5 + i]);
        }

        serialNumber = serialString;
        Serial.printf("Serial Number: %s\n", serialString.c_str());
        serialReceived = true;
    }
    else if (packetID == 0x12) {
        uint8_t userByteZero = rawpacket[5];
        uint8_t userByteOne = rawpacket[6];
        uint8_t userByteTwo = rawpacket[7];
        uint8_t userByteThree = rawpacket[8];

        globalConfig.xBand         = (userByteZero & 0b00000001) != 0;
        globalConfig.kBand         = (userByteZero & 0b00000010) != 0;
        globalConfig.kaBand        = (userByteZero & 0b00000100) != 0;
        globalConfig.laserBand     = (userByteZero & 0b00001000) != 0;
        globalConfig.muteTo        = (userByteZero & 0b00010000) ? "Muted Volume" : "Zero";
        globalConfig.bogeyLockLoud = (userByteZero & 0b00100000) != 0;
        globalConfig.rearMute      = (userByteZero & 0b01000000) != 0;
        globalConfig.kuBand        = (userByteZero & 0b10000000) != 0;

        globalConfig.euro = (userByteOne & 0b00000001) != 0;
        globalConfig.kVerifier = (userByteOne & 0b00000010) != 0;
        globalConfig.rearLaser = (userByteOne & 0b00000100) != 0;
        globalConfig.customFreqEnabled = (userByteOne & 0b00001000) != 0;
        globalConfig.kaAlwaysPrio = (userByteOne & 0b00010000) != 0;
        globalConfig.fastLaserDetection = (userByteOne & 0b00100000) != 0;
        globalConfig.kaSensitivityBit0 = (userByteOne & 0b01000000) ? 1 : 0;
        globalConfig.kaSensitivityBit1 = (userByteOne & 0b10000000) ? 2 : 0;

        int kaSensitivity = globalConfig.kaSensitivityBit0 + globalConfig.kaSensitivityBit1;
        switch (kaSensitivity) {
            case 0:
                globalConfig.kaSensitivity = "Max Range*";
                break;
            case 1:
                globalConfig.kaSensitivity = "Relaxed";
                break;
            case 2:
                globalConfig.kaSensitivity = "2020 Original";
                break;
            case 3:
                globalConfig.kaSensitivity = "Max Range";
                break;
        }

        globalConfig.startupSequence = (userByteTwo & 0b00000001) != 0;
        globalConfig.restingDisplay = (userByteTwo & 0b00000010) != 0;
        globalConfig.bsmPlus = (userByteTwo & 0b00000100) != 0;
        globalConfig.autoMuteBit0 = (userByteTwo & 0b00001000) ? 1 : 0;
        globalConfig.autoMuteBit1 = (userByteTwo & 0b00010000) ? 2 : 0;
        globalConfig.kSensitivityBit0 = (userByteTwo & 0b00100000) ? 1 : 0;
        globalConfig.kSensitivityBit1 = (userByteTwo & 0b01000000) ? 2 : 0;
        globalConfig.mrctPhoto = (userByteTwo & 0b10000000) != 0;

        int kSensitivity = globalConfig.kSensitivityBit0 + globalConfig.kSensitivityBit1;

        switch (kSensitivity) {
            case 0:
                globalConfig.kSensitivity = "2020 Original*";
                break;
            case 1:
                globalConfig.kSensitivity = "Relaxed";
                break;
            case 2:
                globalConfig.kSensitivity = "Max Range";
                break;
            case 3:
                globalConfig.kSensitivity = "2020 Original";
                break;
        }

        uint8_t autoMute = globalConfig.autoMuteBit0 + globalConfig.autoMuteBit1;
        switch (autoMute) {
            case 0:
                globalConfig.autoMute = "Off*";
                break;
            case 1:
                globalConfig.autoMute = "On";
                break;
            case 2:
                globalConfig.autoMute = "On with Unmute 5+";
                break;
            case 3:
                globalConfig.autoMute = "Off";
                break;
        }

        globalConfig.xSensitivityBit0 = (userByteThree & 0b00000001) ? 1 : 0;
        globalConfig.xSensitivityBit1 = (userByteThree & 0b00000010) ? 2 : 0;
        globalConfig.driveSafe3dPhoto = (userByteThree & 0b00000100) != 0;
        globalConfig.driveSafe3dHdPhoto = (userByteThree & 0b00001000) != 0;
        globalConfig.redflexHaloPhoto = (userByteThree & 0b00010000) != 0;
        globalConfig.redflexNK7Photo = (userByteThree & 0b00100000) != 0;
        globalConfig.ekinPhoto = (userByteThree & 0b01000000) != 0;
        globalConfig.photoVerifier = (userByteThree & 0b10000000) != 0;

        int xSensitivity = globalConfig.xSensitivityBit0 + globalConfig.xSensitivityBit1;
        switch (xSensitivity) {
            case 0:
                globalConfig.xSensitivity = "2020 Original*";
                break;
            case 1:
                globalConfig.xSensitivity = "Relaxed";
                break;
            case 2:
                globalConfig.xSensitivity = "Max Range";
                break;
            case 3:
                globalConfig.xSensitivity = "2020 Original";
                break;
        }

        userBytesReceived = true;
    }
    else if (packetID == 0x17) {
        uint8_t aux0 = rawpacket[5];
        uint8_t msbUpper = rawpacket[6];
        uint8_t lsbUpper = rawpacket[7];
        uint8_t msbLower = rawpacket[8];
        uint8_t lsbLower = rawpacket[9];

        std::set<int> receivedSweeps; 
        static int zeroes = 0;
        
        uint8_t sweepIndex = aux0 & 0b00111111;

        int upperBound = combineMSBLSB_v2(msbUpper, lsbUpper);
        int lowerBound = combineMSBLSB_v2(msbLower, lsbLower);
    
        Serial.printf("sweepIndex received: %d | lowerBound: %d | upperBound: %d\n", sweepIndex, lowerBound, upperBound);
        auto exists = std::any_of(globalConfig.sweeps.begin(), globalConfig.sweeps.end(),
        [&](const std::pair<int, int>& sweep) {
            return sweep.first == lowerBound && sweep.second == upperBound;
        });

        if (!exists) {
            if ((lowerBound > 23800 && upperBound < 24300) || 
                (lowerBound > 33300 && upperBound < 36100) || 
                (lowerBound == 0 && upperBound == 0)) {
                
                if (!lowerBound == 0 || !upperBound == 0) {
                    globalConfig.sweeps.emplace_back(lowerBound, upperBound);
                }

                if (lowerBound > 23800 && upperBound < 24300) {
                    k_rcvd = true;
                } else if (lowerBound > 33300 && upperBound < 36100) {
                    ka_rcvd = true;
                } else if (lowerBound == 0 && upperBound == 0) {
                    zeroes++;
                    zero_rcvd = true;
                }
            }
        }
        if (globalConfig.sweeps.size() < globalConfig.maxSweepIndex - zeroes) {
            //Serial.printf("Not all sweeps received (%d/%d), retrying...\n", globalConfig.sweeps.size(), globalConfig.maxSweepIndex + 1);
        } else {
            allSweepDefinitionsReceived = k_rcvd && ka_rcvd && zero_rcvd;

            if (allSweepDefinitionsReceived) {
                unsigned long elapsedMillis = millis() - bootMillis;
                Serial.printf("informational boot complete: %.2f seconds\n", elapsedMillis / 1000.0);
                Serial.printf("heap use after informational boot: %u\n", ESP.getFreeHeap());
            }
        }    
    }
    else if (packetID == 0x20) {
        uint8_t maxSweepIndex = rawpacket[4];
        globalConfig.maxSweepIndex = maxSweepIndex + 1;
        maxSweepIndexReceived = true;
    }
    else if (packetID == 0x23) {
        uint8_t value;
        uint8_t numSections = rawpacket[3];

        switch (numSections) {
            case 6: {
                value = 1;
                processSection_v2(rawpacket, 0);
                break;
            }
            case 11: {
                value = 2;
                processSection_v2(rawpacket, 0);
                processSection_v2(rawpacket, 5);
                break;
            }
            case 16: {
                value = 3;
                processSection_v2(rawpacket, 0);
                processSection_v2(rawpacket, 5);
                processSection_v2(rawpacket, 10);
                break;
            }
            default: {
                value = 0;
            }
        }
        globalConfig.sweepSections = value;
        sweepSectionsReceived = true;
    }
    else if (packetID == 0x38) {
        uint8_t mainV = rawpacket[5];
        uint8_t mutedV = rawpacket[6];

        globalConfig.mainVolume = mainV;
        globalConfig.mutedVolume = mutedV;
        volumeReceived = true;
    }
    else if (packetID == 0x63) {
        uint8_t intPart = rawpacket[5];
        uint8_t decPart = rawpacket[6];
        stats.voltage = intPart + (decPart / 100.0f);
    }
    else if (packetID == 0x66) {
        uint8_t pendingPackets = rawpacket[4] - 1;
        uint8_t p1 = rawpacket[5];
        Serial.printf("infV1Busy; pending packets: %d, first packet ID: 0x%02X\n", pendingPackets, p1);
    }
    return "";
}

/*
the functions below are responsible for generating and sending packets to the v1.
calculating checksums for inbound packets incurs unnecessary overhead therefore
is not done in any of the functions above.
*/

uint8_t Packet::calculateChecksum(const uint8_t *data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

uint8_t* Packet::constructPacket(uint8_t destID, uint8_t sendID, uint8_t packetID, uint8_t *payloadData, uint8_t payloadLength, uint8_t *packet) {
    packet[0] = PACKETSTART;
    packet[1] = 0xD0 + destID;
    packet[2] = 0xE0 + sendID;
    packet[3] = packetID;
    packet[4] = payloadLength;

    if (payloadLength > 1) {
        for (int i = 0; i < payloadLength - 1; i++) {
            packet[5 + i] = payloadData[i];
        }
        packet[5 + payloadLength - 1] = calculateChecksum(packet, 5 + payloadLength - 1);
        packet[5 + payloadLength] = PACKETEND;
    } else {
        packet[4] = 0x01;
        packet[5] = calculateChecksum(packet, 5);
        packet[6] = PACKETEND;
    }
    return packet;
}

uint8_t* Packet::reqTurnOffMainDisplay(uint8_t mode) {
    uint8_t payloadData[] = {mode, mode};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    Serial.println("Sending reqTurnOffMainDisplay packet");
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQTURNOFFMAINDISPLAY, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqStartAlertData() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    Serial.println("Sending reqStartAlertData packet");
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQSTARTALERTDATA, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqVersion() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]); 
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQVERSION, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqSweepSections() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQSWEEPSECTIONS, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqMaxSweepIndex() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQMAXSWEEPINDEX, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqAllSweepDefinitions() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQALLSWEEPDEFINITIONS, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqSerialNumber() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQSERIALNUMBER, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqTurnOnMainDisplay() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    Serial.println("Sending reqTurnOnMainDisplay packet");
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQTURNONMAINDISPLAY, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqMuteOn() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQMUTEON, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqMuteOff() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQMUTEOFF, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqChangeMode(uint8_t mode) {
    uint8_t payloadData[] = {mode, mode};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQCHANGEMODE, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqBatteryVoltage() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQBATTERYVOLTAGE, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqCurrentVolume() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQCURRENTVOLUME, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqUserBytes() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQUSERBYTES, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqSavvyStatus() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQSAVVYSTATUS, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqVehicleSpeed() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQVEHICLESPEED, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}