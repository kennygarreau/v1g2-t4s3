#include "ble.h"
#include "v1_packet.h"
#include "v1_config.h"
#include "tft_v2.h"
#include "ui/ui.h"
#include "ui/actions.h"
#include <vector>
#include <LilyGo_AMOLED.h>
#include <set>


struct BandDirection {
    const char* band;
    const char* direction; };
struct alertByte {
    int count;
    int index; };
static std::string dirValue, bandValue;
static std::string lastPayload = "";
static std::string lastinfPayload = "";
int frontStrengthVal, rearStrengthVal;
bool priority, junkAlert, alertPresent, muted, remoteAudio, savvy;
static int alertCountValue, alertIndexValue;
float freqGhz;
alertsVector alertTable;
Config globalConfig;
std::string prio_alert_freq = "";
static char current_alerts[MAX_ALERTS][32];
static int num_current_alerts = 0;
uint8_t activeBands = 0;
uint8_t lastReceivedBands = 0;

static bool k_rcvd = false;
static bool ka_rcvd = false;
static bool zero_rcvd = false;

extern void requestMute();
uint8_t packet[10];

PacketDecoder::PacketDecoder(const std::string& packet) : packet(packet) {}

/* 
    hexToDecimal is an overloaded function which will convert a hex input to a decimal
    this is required for the bar mapping functions below
*/

int hexToDecimal(char hex) {
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    } else if (hex >= 'a' && hex <= 'f') {
        return hex - 'a' + 10;
    } else if (hex >= 'A' && hex <= 'F') {
        return hex - 'A' + 10;
    }
    return -1;
}

int hexToDecimal(const std::string& hexStr) {
    int decimalValue = 0;
    for (size_t i = 0; i < hexStr.length(); ++i) {
        int digit = hexToDecimal(hexStr[i]);
        if (digit == -1) {
            return -1;
        }
        decimalValue += digit * pow(16, hexStr.length() - 1 - i);
    }
    return decimalValue;
}

std::string hexToAscii(const std::string& hexStr) {
    std::string asciiStr;
    for (size_t i = 0; i < hexStr.length(); i += 2) {
        // Convert each pair of hex characters to a byte (uint8_t)
        std::string byteStr = hexStr.substr(i, 2);
        char byte = static_cast<char>(std::stoi(byteStr, nullptr, 16));
        
        if (std::isprint(byte)) {
            asciiStr += byte;  // Append the ASCII character to the result string
        } else {
            asciiStr += '.';  // Replace non-printable characters with a dot
        }
    }
    return asciiStr;
}

void updateActiveBands(uint8_t band) {
    activeBands |= band;
}

void clearInactiveBands(uint8_t newBandData) {
    activeBands &= (lastReceivedBands | newBandData); 
}

void PacketDecoder::clearTableAlerts() {
    set_var_showAlertTable(false);
}

void PacketDecoder::clearInfAlerts() {
    set_var_prio_alert_freq("");
    set_var_prioBars(0);
    set_var_kaAlert(false);
    set_var_kAlert(false);
    set_var_xAlert(false);
    set_var_laserAlert(false);
    set_var_muted(false);
    set_var_arrowPrioFront(false);
    set_var_arrowPrioRear(false);
    set_var_arrowPrioSide(false);
    std::fill(std::begin(blink_enabled), std::end(blink_enabled), false);

    ui_tick();
    lv_task_handler();
}

int mapXToBars(const std::string& hex) {
    if (hex.empty() || hex.length() > 2 || !std::all_of(hex.begin(), hex.end(), ::isxdigit)) {
        //Serial.println("Invalid X hex strength input");
        return 0;
    }

    int decimalValue = std::stoi(hex, 0, 16);
    static constexpr uint8_t thresholds[] = {0x00, 0x95, 0x9F, 0xA9, 0xB3, 0xBC, 0xC4, 0xCF, 0xFF};

    for (int i = 0; i < 8; ++i) {
        if (decimalValue <= thresholds[i]) return i;
    }
    return -1;
}

int mapKToBars(const std::string& hex) {
    if (hex.empty() || hex.length() > 2 || !std::all_of(hex.begin(), hex.end(), ::isxdigit)) {
        //Serial.printf("Invalid K hex strength input: %s", hex.c_str());
        return 0;
    }

    int decimalValue = std::stoi(hex, 0, 16);
    static constexpr uint8_t thresholds[] = {0x00, 0x87, 0x8F, 0x99, 0xA3, 0xAD, 0xB7, 0xC1, 0xFF};
    
    for (int i = 0; i < 8; ++i) {
        if (decimalValue <= thresholds[i]) return i;
    }
    return -1;
}

int mapKaToBars(const std::string& hex) {
    if (hex.empty() || hex.length() > 2 || !std::all_of(hex.begin(), hex.end(), ::isxdigit)) {
        //Serial.println("Invalid Ka hex strength input");
        return 0;
    }

    int decimalValue = std::stoi(hex, 0, 16);
    static constexpr uint8_t thresholds[] = {0x00, 0x8F, 0x96, 0x9D, 0xA4, 0xAB, 0xB2, 0xB9, 0xFF};
    
    for (int i = 0; i < 8; ++i) {
        if (decimalValue <= thresholds[i]) return i;
    }
    return -1;
}

int combineMSBLSB(const std::string& msb, const std::string& lsb) {
    int msbDecimal = hexToDecimal(msb);
    int lsbDecimal = hexToDecimal(lsb);
    if (msbDecimal == -1 || lsbDecimal == -1) {
        return 0;
    }
    return (msbDecimal * 256) + lsbDecimal;
}

void processSection(const std::string& packet, int offset) {
    int sweepDefIndexNum = std::stoi(packet.substr(offset, 2), nullptr, 16);
    int sectionCount = sweepDefIndexNum & 0b00001111;
    int sectionIndex = (sweepDefIndexNum & 0b11110000) >> 4;

    int upperBound = combineMSBLSB(packet.substr(offset + 2, 2), packet.substr(offset + 4, 2));
    int lowerBound = combineMSBLSB(packet.substr(offset + 6, 2), packet.substr(offset + 8, 2));

    Serial.printf("section %d: lower edge: %d, upper edge: %d\n", sectionIndex, lowerBound, upperBound);
    globalConfig.sections.emplace_back(lowerBound, upperBound);
}

// User Bytes here are valid from V4.1018+
void decodeByteZero(const std::string& userByte) {
    if (!userByte.empty()) {
        uint8_t byteValue = (uint8_t) strtol(userByte.c_str(), nullptr, 16);

        globalConfig.xBand = byteValue & 0b00000001;
        globalConfig.kBand = byteValue & 0b00000010;
        globalConfig.kaBand = byteValue & 0b00000100;
        globalConfig.laserBand = byteValue & 0b00001000;
        globalConfig.muteTo = (byteValue & 0b00010000) ? "Muted Volume" : "Zero";
        globalConfig.bogeyLockLoud = byteValue & 0b00100000;
        globalConfig.rearMute = byteValue & 0b01000000;
        globalConfig.kuBand = byteValue & 0b10000000;
    }
}

void decodeByteOne(const std::string& userByte) {
    if (!userByte.empty()) {
        uint8_t byteValue = (uint8_t) strtol(userByte.c_str(), nullptr, 16);
        
        globalConfig.euro = byteValue & 0b00000001;
        globalConfig.kVerifier = byteValue & 0b00000010;
        globalConfig.rearLaser = byteValue & 0b00000100;
        globalConfig.customFreqEnabled = byteValue & 0b00001000;
        globalConfig.kaAlwaysPrio = byteValue & 0b00010000;
        globalConfig.fastLaserDetection = byteValue & 0b00100000;
        globalConfig.kaSensitivityBit0 = (byteValue & 0b01000000) ? 1 : 0;
        globalConfig.kaSensitivityBit1 = (byteValue & 0b10000000) ? 2 : 0;

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
    }
}

void decodeByteTwo(const std::string& userByte) {
    if (!userByte.empty()) {
        uint8_t byteValue = (uint8_t) strtol(userByte.c_str(), nullptr, 16);
        
        globalConfig.startupSequence = byteValue & 0b00000001;
        globalConfig.restingDisplay = byteValue & 0b00000010;
        globalConfig.bsmPlus = byteValue & 0b00000100;
        globalConfig.autoMuteBit0 = (byteValue & 0b00001000) ? 1 : 0;
        globalConfig.autoMuteBit1 = (byteValue & 0b00010000) ? 2 : 0;

        int autoMute = globalConfig.autoMuteBit0 + globalConfig.autoMuteBit1;
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
        userBytesReceived = true;
    }
}

BandArrowData processBandArrow(const std::string& bandArrow) {
    BandArrowData data = {false};

    try {
        if (!bandArrow.empty()) {
            int bandArrowInt = std::stoi(bandArrow, nullptr, 16);

            data.laser = (bandArrowInt & 0b00000001) != 0;
            data.ka = (bandArrowInt & 0b00000010) != 0;
            data.k = (bandArrowInt & 0b00000100) != 0;
            data.x = (bandArrowInt & 0b00001000) != 0;
            data.muteIndicator = (bandArrowInt & 0b00010000) != 0;
            data.front = (bandArrowInt & 0b00100000) != 0;
            data.side = (bandArrowInt & 0b01000000) != 0;
            data.rear = (bandArrowInt & 0b10000000) != 0;

            clearInactiveBands(bandArrowInt);
        } else {
            Serial.println("Error: Invalid bandArrow length");
        }
        } catch (const std::exception& e) {
        // Serial.print("Error in processBandArrow: ");
        // Serial.println(e.what());
    }
    
    return data;
}

void compareBandArrows(const BandArrowData& arrow1, const BandArrowData& arrow2) {
    set_var_muted(arrow1.muteIndicator);

    bool anyBandActive = false;

    if (arrow1.ka != arrow2.ka) {
        enable_blinking(BLINK_KA);
        Serial.print("== blink ka ");
        //updateActiveBands(0b00000010);
    }
    if (!blink_enabled[BLINK_KA] && arrow1.ka) {
        set_var_kaAlert(arrow1.ka);
        updateActiveBands(0b00000010);
        anyBandActive = true;
    }

    if (arrow1.k != arrow2.k) {
        enable_blinking(BLINK_K);
        Serial.print("== blink k ");
        //updateActiveBands(0b00000100);
    }
    if (!blink_enabled[BLINK_K] && arrow1.k) {
        set_var_kAlert(arrow1.k);
        updateActiveBands(0b00000100);
        anyBandActive = true;
    }

    if (arrow1.x != arrow2.x && globalConfig.xBand) {
        enable_blinking(BLINK_X);
        Serial.print("== blink x ");
        //updateActiveBands(0b00001000);
    }
    if (!blink_enabled[BLINK_X] && arrow1.x && globalConfig.xBand) {
        set_var_xAlert(arrow1.x);
        updateActiveBands(0b00001000);
        anyBandActive = true;
    }

    if (arrow1.front != arrow2.front) {
        enable_blinking(BLINK_FRONT);
        Serial.printf("== blink front == \n");
    } else if (!blink_enabled[BLINK_FRONT]) {
        set_var_arrowPrioFront(arrow1.front);
    }

    if (arrow1.side != arrow2.side) {
        enable_blinking(BLINK_SIDE);
        Serial.printf("== blink side == \n");
    } else if (!blink_enabled[BLINK_SIDE]) {
        set_var_arrowPrioSide(arrow1.side);
    }

    if (arrow1.rear != arrow2.rear) {
        enable_blinking(BLINK_REAR);
        Serial.printf("== blink rear == \n");
    } else if (!blink_enabled[BLINK_REAR]) {
        set_var_arrowPrioRear(arrow1.rear);
    }
        
    if (arrow1.laser) {
        // TODO: this might need a logic change if a laser alert comes in while there's a mute (gray) condition
        if (muted) {
            set_var_muted(false);
            reqMuteOff();
        }
        enable_blinking(BLINK_LASER);
        set_var_prio_alert_freq("LASER");
        set_var_prioBars(7);
        updateActiveBands(0b00000001);
        anyBandActive = true;
        Serial.println("== blink laser == ");
        set_var_laserAlert(arrow1.laser);

        if (arrow1.front || arrow1.rear) {
            enable_blinking(arrow1.front ? BLINK_FRONT : BLINK_REAR);
        }
    }

    if (!anyBandActive) {
        start_clear_inactive_bands_timer();
    }
}

/* 
Execute if we successfully write reqStartAlertData to clientWriteUUID
*/
void PacketDecoder::decodeAlertData(const alertsVector& alerts, int lowSpeedThreshold, int currentSpeed) {
    static unsigned long startTimeMillis = millis();

    frontStrengthVal = 0;
    rearStrengthVal = 0;

    std::vector<AlertTableData> alertDataList;

    for (int i = 0; i < alerts.size(); i++) {
        if (alerts[i].length() < 14) {
            Serial.printf("Error: alert string too short: %s\n", alerts[i].c_str());
            continue;
        }

        std::string alertIndexStr = alerts[i].substr(0, 2);

        if (!alertIndexStr.empty()) {
            try {
                int alertIndex = std::stoi(alertIndexStr, nullptr, 16);
    
                alertCountValue = alertIndex & 0b00001111;
                alertIndexValue = (alertIndex & 0b11110000) >> 4;
            } catch (const std::exception& e) {
                // Serial.print("Error parsing alert index: ");
                // Serial.println(alertIndexStr.c_str());
            }
        } else {
            Serial.print("Invalid alertIndexStr: ");
            Serial.println(alertIndexStr.c_str());
        }

        std::string freqMSB = alerts[i].substr(2, 2);
        std::string freqLSB = alerts[i].substr(4, 2);

        int freqMhz = combineMSBLSB(freqMSB, freqLSB);
        freqGhz = static_cast<float>(freqMhz) / 1000.0f;

        std::string frontStrength = alerts[i].substr(6, 2);
        std::string rearStrength = alerts[i].substr(8, 2);
        std::string auxByte = alerts[i].substr(12, 2);

        std::string bandArrowDef = alerts[i].substr(10, 2);

        if (!bandArrowDef.empty()) {
            try {
                int bandArrow = std::stoi(bandArrowDef, nullptr, 16);
                bandValue = "Unknown";
                dirValue = "Unknown";
            
                switch (bandArrow & 0b00011111) { // Mask the band bits
                    case 0b00000001: bandValue = "LASER"; break;
                    case 0b00000010: bandValue = "Ka"; break;
                    case 0b00000100: bandValue = "K"; break;
                    case 0b00001000: bandValue = "X"; break;
                    case 0b00010000: bandValue = "Ku"; break;
                }
            
                switch (bandArrow & 0b11100000) { // Mask the direction bits
                    case 0b00100000: dirValue = "FRONT"; break;
                    case 0b01000000: dirValue = "SIDE"; break;
                    case 0b10000000: dirValue = "REAR"; break;
                }
            } catch (const std::exception& e) {
                // anything to be done here?
            }
        }

        priority = (auxByte == "80");
        junkAlert = (auxByte == "40");

        /* after this there should be no substring processing; we should only focus on painting the display */
        // paint the alert table arrows
        if (bandValue == "X" && globalConfig.xBand) {
            frontStrengthVal = mapXToBars(frontStrength);
            rearStrengthVal = mapXToBars(rearStrength);
        } 
        else if (bandValue == "K" || bandValue == "Ku") {
            frontStrengthVal = mapKToBars(frontStrength);
            rearStrengthVal = mapKToBars(rearStrength);
        }
        else if (bandValue == "Ka") {
            frontStrengthVal = mapKaToBars(frontStrength);
            rearStrengthVal = mapKaToBars(rearStrength);        
        }
        // paint the priority alert
        if (priority && bandValue != "LASER") {
            if (!muted && gpsAvailable) {
                if (currentSpeed <= lowSpeedThreshold) {
                    Serial.println("SilentRide requesting mute");
                    requestMute();
                }
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

        if (!priority) {
            bool found = false;
            for (auto& alertData : alertDataList) {
                if (alertData.alertCount == alertCountValue) {
                    alertData.frequencies[alertData.freqCount++] = freqGhz;
                    break;
                }
            }
    
            int barCount = get_var_prioBars();
            if (!found) {
                AlertTableData newAlertData = {alertCountValue, {freqGhz}, {dirValue}, barCount, 1};
                alertDataList.push_back(newAlertData);
                found = true;
            }
        }

        unsigned long elapsedTimeMillis = millis() - startTimeMillis;
        //Serial.printf("decode time: %lu\n", elapsedTimeMillis);
        if (freqGhz > 0) {
            std::string decodedPayload = "INDX:" + std::to_string(alertIndexValue) +
                                        " TOTL:" + std::to_string(alertCountValue) +
                                        " FREQ:" + std::to_string(freqGhz) +
                                        " FSTR:" + std::to_string(frontStrengthVal) +
                                        " RSTR:" + std::to_string(rearStrengthVal) +
                                        " BAND:" + bandValue +
                                        " BDIR:" + dirValue +
                                        " PRIO:" + std::to_string(priority) +
                                        " JUNK:" + std::to_string(junkAlert) +
                                        " SPEED: " + std::to_string(gpsData.speed);
                                        //+ " decodeAlert(ms): " + std::to_string(elapsedTimeMillis);
            Serial.println(("respAlertData: " + decodedPayload).c_str());
        }
    }
    set_var_alertCount(alertCountValue);
    set_var_alertTableSize(alertDataList.size());
    set_var_frequencies(alertDataList);
}

/*  decode operation, passed from loop() - based on the packet ID.
    ID 31 (infDisplayData): route to decodeDisplayData
    ID 43 (respAlertData): direct decode 
*/
std::string PacketDecoder::decode(int lowSpeedThreshold, int currentSpeed) {
    //unsigned long startTimeMillis = millis();

    if (packet.compare(0, 2, "AA") != 0) {
        return "err SOF";
    }
    
    if (packet.compare(packet.size() - 2, 2, "AB") != 0) {
        return "err EOF";
    }

    std::string packetID = packet.substr(6, 2);

    // infDisplayData
    if (packetID == "31") {
        /*  
            infDisplayData
            Packet length: 28
            Control main arrow paint and blink
        */
       std::string payload = packet.substr(10, 16);

       if (settings.displayTest || payload != lastinfPayload) {
        std::string bandArrow1 = (payload.length() >= 8) ? payload.substr(6, 2) : "";
        std::string bandArrow2 = (payload.length() >= 10) ? payload.substr(8, 2) : "";

        if (!bandArrow1.empty() && !bandArrow2.empty()) {
            BandArrowData arrow1Data = processBandArrow(bandArrow1);
            BandArrowData arrow2Data = processBandArrow(bandArrow2);

            compareBandArrows(arrow1Data, arrow2Data);
        }
        else {
            Serial.println("DEBUG: bandArrow empty");
            //clearInfAlerts();
            //clearTableAlerts();
            //alertPresent = false;
        }

        std::string auxByte0 = packet.substr(packet.length() - 10, 2);
        std::string auxByte1 = packet.substr(packet.length() - 8, 2);
        std::string auxByte2 = packet.substr(packet.length() - 6, 2);
        if (!auxByte0.empty()) {
            try {
                int auxByte0Int = std::stoi(auxByte0, nullptr, 16);
                bool softMute = (auxByte0Int & 0b00000001) ? 1 : 0;
                if (softMute) {
                    Serial.printf("soft mute status: %d | muted set to: %d\n", softMute, muted);
                }
            } catch (const std::exception& e) {}
        }
        if (!auxByte1.empty()) {
            try {
                int auxByte1Int = std::stoi(auxByte1, nullptr, 16);

                int modeBit0 = (auxByte1Int & 0b00000100) ? 1 : 0;
                int modeBit1 = (auxByte1Int & 0b00001000) ? 2 : 0;
                int mode = modeBit0 + modeBit1;
                int mutedReason = (auxByte1Int & 0b00010000) ? 1 : 0;
                //Serial.printf("Muted reason: %d\n", mutedReason);

                switch(mode) {
                    case 0:
                        globalConfig.mode = "Invalid Mode";
                        globalConfig.defaultMode = "I";
                        break;
                    case 1:
                        globalConfig.mode = "ALL BOGEYS";
                        globalConfig.defaultMode = "A";
                        break;
                    case 2:
                        globalConfig.mode = "LOGIC";
                        globalConfig.defaultMode = "c";
                        break;
                    case 3:
                        globalConfig.mode = "ADV LOGIC";
                        globalConfig.defaultMode = "L";
                        break;
                }
            } catch (const std::exception& e) {}
        }
        //Serial.printf("infDisplayData loop time: %lu\n", millis() - startTimeMillis);
        lastinfPayload = payload;
    }
    }
    // respAlertData
    else if (packetID == "43") {
        /*  
            respAlertData
            Packet length: 26
            Alert table, priority frequency & bars
        */
        std::string payload = packet.substr(10, packet.length() - 12);
        std::string alertC = payload.substr(0, 2);
        
        if (alertC == "00") {
            if (alertPresent) {
                Serial.println("alertC 00 && alertPresent, clearing alerts");
                clearTableAlerts();
                clearInfAlerts();
                alertPresent = false;
            }
            return "";
        } 
        else {
            lastPayload = payload;
            std::string alertIndexStr = packet.substr(10, 2);

            if (!alertIndexStr.empty()) {
                try {
                    int alertIndex = std::stoi(alertIndexStr, nullptr, 16);

                    alertCountValue = alertIndex & 0b00001111;
                    alertIndexValue = (alertIndex & 0b11110000) >> 4;
                } catch (const std::exception& e) {}
            } else {
                Serial.println("Warning: alertIndexStr is empty!");
            }
            alertTable.push_back(payload);

            // check if the alertTable vector size is more than or equal to the tableSize (alerts.count) extracted from alertByte
            if (alertTable.size() >= alertCountValue || alertTable.size() == MAX_ALERTS + 1) {
                alertPresent = true;
                set_var_showAlertTable(alertCountValue > 1);
                decodeAlertData(alertTable, lowSpeedThreshold, currentSpeed);
                alertTable.clear();
            } 
        //Serial.printf("respAlertData loop time: %lu\n", millis() - startTimeMillis);
        }
    }
    // respVersion
    else if (packetID == "02"){
        try {
            std::string versionID = hexToAscii(packet.substr(10, 2));
            std::string majorVersion = hexToAscii(packet.substr(12, 2));
            std::string minorVersion = hexToAscii(packet.substr(16, 2));
            std::string revisionDigitOne = hexToAscii(packet.substr(18, 2));
            std::string revisionDigitTwo = hexToAscii(packet.substr(20, 2));
            std::string controlNumber = hexToAscii(packet.substr(22, 2));

            if (versionID == "V") {
                std::string versionString = majorVersion + "." + minorVersion + revisionDigitOne + revisionDigitTwo + controlNumber;
                softwareRevision = versionString;
                Serial.printf("Software Version: %s\n", versionString.c_str());
                versionReceived = true;
            } else if (versionID == "R") {
                remoteAudio = true;
            } else if (versionID == "S") {
                savvy = true;
            } else {
                Serial.printf("Found component: %s", versionID);
            }
        } catch (const std::exception& e) {}
    }
    // respSerialNumber
    else if (packetID == "04"){
        try {
            std::string serialNum1 = hexToAscii(packet.substr(10, 2));
            std::string serialNum2 = hexToAscii(packet.substr(12, 2));
            std::string serialNum3 = hexToAscii(packet.substr(14, 2));
            std::string serialNum4 = hexToAscii(packet.substr(16, 2));
            std::string serialNum5 = hexToAscii(packet.substr(18, 2));
            std::string serialNum6 = hexToAscii(packet.substr(20, 2));
            std::string serialNum7 = hexToAscii(packet.substr(22, 2));
            std::string serialNum8 = hexToAscii(packet.substr(24, 2));
            std::string serialNum9 = hexToAscii(packet.substr(26, 2));
            std::string serialNum10 = hexToAscii(packet.substr(28, 2));

            std::string serialString = serialNum1 + serialNum2 + serialNum3 + serialNum4 + serialNum5 + serialNum6 + 
                serialNum7 + serialNum8 + serialNum9 + serialNum10;
            serialNumber = serialString;
            Serial.printf("Serial Number: %s\n", serialString.c_str());
            serialReceived = true;
        } catch (const std::exception& e) {
            // anything to be done here?
        }
    }
    // respUserBytes
    else if (packetID == "12") {
        std::string userByteZero = packet.substr(10, 2);
        std::string userByteOne = packet.substr(12, 2);
        std::string userByteTwo = packet.substr(14, 2);

        decodeByteZero(userByteZero);
        decodeByteOne(userByteOne);
        decodeByteTwo(userByteTwo);
    }
    // respSweepDefinition
    else if (packetID == "17") {
        std::string aux0 = packet.substr(10, 2);
        std::string msbUpper = packet.substr(12, 2);
        std::string lsbUpper = packet.substr(14, 2);
        std::string msbLower = packet.substr(16, 2);
        std::string lsbLower = packet.substr(18, 2);
        std::set<int> receivedSweeps; 
        static int zeroes = 0;
    
        try {
            int sweepIndex = std::stoi(aux0, nullptr, 16);
            sweepIndex -= 128;
            int upperBound = combineMSBLSB(msbUpper, lsbUpper);
            int lowerBound = combineMSBLSB(msbLower, lsbLower);
    
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
        }
        catch (const std::exception& e) {
            Serial.printf("caught exception processing allSweepDefinitions: %s\n", e.what());
        }

        if (globalConfig.sweeps.size() < globalConfig.maxSweepIndex - zeroes) {
            //Serial.printf("Not all sweeps received (%d/%d), retrying...\n", globalConfig.sweeps.size(), globalConfig.maxSweepIndex + 1);
        } else {
            allSweepDefinitionsReceived = k_rcvd && ka_rcvd && zero_rcvd;

            if (allSweepDefinitionsReceived) {
                unsigned long elapsedMillis = millis() - bootMillis;
                Serial.printf("informational boot complete: %.2f seconds\n", elapsedMillis / 1000.0);
            }
        }     
    }    
    // respMaxSweepIndex
    else if (packetID == "20") {
        std::string maxSweepIndex = packet.substr(10, 2);
        try {
            int maxSweepIndexInt = std::stoi(maxSweepIndex, nullptr, 16);
            globalConfig.maxSweepIndex = maxSweepIndexInt + 1;
            maxSweepIndexReceived = true;
        }
        catch (const std::exception& e) {
        } 
    }
    // respSweepSections
    else if (packetID == "23") {
        int value;
        std::string numSections = packet.substr(8, 2);
        
        try {
            int numSectionsInt = std::stoi(numSections, nullptr, 16);
            switch (numSectionsInt) {
                case 6: {
                    value = 1;
                    processSection(packet, 10);
                    break;
                }
                case 11: {
                    value = 2;
                    processSection(packet, 10);
                    processSection(packet, 20);
                    break;
                }
                case 16: {
                    value = 3;
                    processSection(packet, 10);
                    processSection(packet, 20);
                    processSection(packet, 30);
                    break;
                }
                default: {
                    value = 0;
                }
            }
            
            globalConfig.sweepSections = value;
            sweepSectionsReceived = true;
        }
        catch (const std::exception& e) {
        } 
    }
    // respCurrentVolume
    else if (packetID == "38") {
        int mainVol = 0, mutedVol = 0;
        try {
            std::string mainV = packet.substr(10, 2);
            std::string mutedV = packet.substr(12, 2);

            if (!mainV.empty() && !mutedV.empty()) {
                mainVol = std::stoi(mainV, nullptr, 16);
                mutedVol = std::stoi(mutedV, nullptr, 16);
                globalConfig.mainVolume = mainVol;
                globalConfig.mutedVolume = mutedVol;
                volumeReceived = true;
            }
        } catch (const std::exception& e) {

        }
    }
    // respBatteryVoltage
    else if (packetID == "63") {
        int voltageInt = 0, voltageDec = 0;
        try {
            std::string intPart = packet.substr(10, 2);
            std::string decPart = packet.substr(12, 2);
        
            if (!intPart.empty() && !decPart.empty()) {
                voltageInt = std::stoi(intPart, nullptr, 16);
                voltageDec = std::stoi(decPart, nullptr, 16);
                gpsData.voltage = voltageInt + (voltageDec / 100.0f);
            }
        } catch (const std::exception& e) {
            Serial.println("caught an exception during respBatteryVoltage");
        }
    }
    // respUnsupportedPacket
    else if (packetID == "64") {
        Serial.println("respUnsupportedPacket");
    }
    // respRequestNotProcessed
    else if (packetID == "65") {
        Serial.println("respRequestNotProcessed");
    }
    // infV1Busy
    else if (packetID == "66") {
        std::string pl = packet.substr(8, 2);
        try {
            int plInt = std::stoi(pl, nullptr, 10) - 1;
            std::string pendingPacket = packet.substr(10, 2);
            Serial.printf("infV1Busy; pending packets: %d, first packet ID: %s\n", plInt, pendingPacket.c_str());
        }
        catch (const std::exception& e) {
        }   
    }
    // respDataError
    else if (packetID == "67") {
        Serial.println("respDataError encountered");
    }
    // respSavvyStatus
    else if (packetID == "72") {
        Serial.println("respSavvyStatus encountered");
    }
    // respVehicleSpeed
    else if (packetID == "74") {
        Serial.println("respVehicleSpeed encountered");
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