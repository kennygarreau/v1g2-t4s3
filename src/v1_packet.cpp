#include "v1_packet.h"
#include "v1_config.h"
#include "tft_v2.h"
#include "ui/ui.h"
#include "ui/actions.h"
#include <vector>
#include <LilyGo_AMOLED.h>

struct BandDirection {
    const char* band;
    const char* direction; };
struct alertByte {
    int count;
    int index; };
static std::string directionValue, bandValue;
static std::string lastPayload = "";
static std::string lastinfPayload = "";
int frontStrengthVal, rearStrengthVal;
bool priority, junkAlert;
static int alertCountValue, alertIndexValue;
float freqGhz;
alertsVector alertTable;
Config globalConfig;
std::string prio_alert_freq = "";
static char current_alerts[MAX_ALERTS][32];
static int num_current_alerts = 0;

extern void requestMute();
bool muted = false;
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

void PacketDecoder::clearTableAlerts() {
    set_var_prio_alert_freq("");
    set_var_prioBars(0);
    set_var_alertPresent(false);
}

void PacketDecoder::clearInfAlerts() {
    set_var_kaAlert(false);
    set_var_kAlert(false);
    set_var_xAlert(false);
    set_var_laserAlert(false);
    set_var_muted(false);
    set_var_arrowPrioFront(false);
    set_var_arrowPrioRear(false);
    set_var_arrowPrioSide(false);
    for (int i = 0; i < MAX_BLINK_IMAGES; i++) {
        blink_enabled[i] = false;
    }
}

int mapXToBars(const std::string& hex) {
    int decimalValue = 0;
    try {
        if (hex.empty()) {
            throw std::invalid_argument("Empty string");
        }
        decimalValue = std::stoi(hex, nullptr, 16);
    } catch (const std::invalid_argument &e) {
        Serial.printf("Invalid argument in std::stoi: %s\n", e.what());
    } catch (const std::out_of_range &e) {
        Serial.printf("Out of range error in std::stoi: %s\n", e.what());
    }

    if (decimalValue == 0x00) {return 0;}
    else if (decimalValue <= 0x95) {return 1;}
    else if (decimalValue <= 0x9F) {return 2;}
    else if (decimalValue <= 0xA9) {return 3;}
    else if (decimalValue <= 0xB3) {return 4;}
    else if (decimalValue <= 0xBC) {return 5;}
    else if (decimalValue <= 0xC4) {return 6;}
    else if (decimalValue <= 0xCF) {return 7;}
    else if (decimalValue <= 0xFF) {return 8;}
    else {return -1;}
}

int mapKToBars(const std::string& hex) {
    int decimalValue = 0;
    try {
        if (hex.empty()) {
            throw std::invalid_argument("Empty string");
        } 
        decimalValue = std::stoi(hex, nullptr, 16);
    } catch (const std::invalid_argument &e) {
        Serial.printf("Invalid argument in std::stoi: %s\n", e.what());
    } catch (const std::out_of_range &e) {
        Serial.printf("Out of range error in std::stoi: %s\n", e.what());
    }

    if (decimalValue == 0x00) {return 0;}
    else if (decimalValue <= 0x87) {return 1;}
    else if (decimalValue <= 0x8F) {return 2;}
    else if (decimalValue <= 0x99) {return 3;}
    else if (decimalValue <= 0xA3) {return 4;}
    else if (decimalValue <= 0xAD) {return 5;}
    else if (decimalValue <= 0xB7) {return 6;}
    else if (decimalValue <= 0xC1) {return 7;}
    else if (decimalValue <= 0xFF) {return 8;}
    else {return -1;}
}

int mapKaToBars(const std::string& hex) {
    int decimalValue = 0;
    try {
        if (hex.empty()) {
            throw std::invalid_argument("Empty string");
        }
        decimalValue = std::stoi(hex, nullptr, 16);
    } catch (const std::invalid_argument &e) {
        Serial.printf("Invalid argument in std::stoi: %s\n", e.what());
    } catch (const std::out_of_range &e) {
        Serial.printf("Out of range error in std::stoi: %s\n", e.what());
    }

    if (decimalValue == 0x00) {return 0;}
    else if (decimalValue <= 0x8F) {return 1;}
    else if (decimalValue <= 0x96) {return 2;}
    else if (decimalValue <= 0x9D) {return 3;}
    else if (decimalValue <= 0xA4) {return 4;}
    else if (decimalValue <= 0xAB) {return 5;}
    else if (decimalValue <= 0xB2) {return 6;}
    else if (decimalValue <= 0xB9) {return 7;}
    else if (decimalValue <= 0xFF) {return 8;}
    else {return -1;}
}

int combineMSBLSB(const std::string& msb, const std::string& lsb) {
    int msbDecimal = hexToDecimal(msb);
    int lsbDecimal = hexToDecimal(lsb);
    if (msbDecimal == -1 || lsbDecimal == -1) {
        return -1;
    }
    return (msbDecimal * 256) + lsbDecimal;
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
        globalConfig.customFreqDisabled = byteValue & 0b00001000;
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
    }
}

BandArrowData processBandArrow(const std::string& bandArrow) {
    BandArrowData data = {false};

    try {
        if (!bandArrow.empty()) {
            int bandArrowInt = std::stoi(bandArrow, nullptr, 16);

            data.laser = bandArrowInt & 0b00000001;
            data.ka = bandArrowInt & 0b00000010;
            data.k = bandArrowInt & 0b00000100;
            data.x = bandArrowInt & 0b00001000;
            data.muteIndicator = bandArrowInt & 0b00010000;
            data.front = bandArrowInt & 0b00100000;
            data.side = bandArrowInt & 0b01000000;
            data.rear = bandArrowInt & 0b10000000;
        } else {
            Serial.println("Error: Invalid bandArrow length");
        }
        } catch (const std::exception& e) {
        Serial.print("Error in processBandArrow: ");
        Serial.println(e.what());
    }

    return data;
}

void compareBandArrows(const BandArrowData& arrow1, const BandArrowData& arrow2) {
    set_var_muted(arrow1.muteIndicator);
    
    if (arrow1.laser) {
        enable_blinking(BLINK_LASER);
        set_var_prio_alert_freq("LASER");
        set_var_prioBars(6);
    } else {
        set_var_laserAlert(arrow1.laser);
    }

    if (arrow1.ka != arrow2.ka) {
        enable_blinking(BLINK_KA);
    }
    else if (!blink_enabled[BLINK_KA]) {
        set_var_kaAlert(arrow1.ka);
    }

    if (arrow1.k != arrow2.k) {
        enable_blinking(BLINK_K);
    }
    else if (!blink_enabled[BLINK_K]) {
        set_var_kAlert(arrow1.k);
    }

    if (arrow1.x != arrow2.x) {
        enable_blinking(BLINK_X);
    } else if (!blink_enabled[BLINK_X]) {
        set_var_xAlert(arrow1.x);
    }

    if (arrow1.front != arrow2.front) {
        enable_blinking(BLINK_FRONT);
        Serial.println("Blink front arrow");
    } else if (!blink_enabled[BLINK_FRONT]) {
        set_var_arrowPrioFront(arrow1.front);
    }

    if (arrow1.side != arrow2.side) {
        enable_blinking(BLINK_SIDE);
        Serial.println("Blink side");
    } else if (!blink_enabled[BLINK_SIDE]) {
        set_var_arrowPrioSide(arrow1.side);
    }

    if (arrow1.rear != arrow2.rear) {
        enable_blinking(BLINK_REAR);
        Serial.println("Blink rear");
    } else if (!blink_enabled[BLINK_REAR]) {
        set_var_arrowPrioRear(arrow1.rear);
    }

    // blink_enabled[BLINK_REAR] = (arrow1.rear != arrow2.rear);
    // if (blink_enabled[BLINK_REAR]) {
    //     Serial.println("blink rear");
    // } else {
    //     set_var_arrowPrioRear(arrow1.rear);
    // }
}

/* 
Execute if we successfully write reqStartAlertData to clientWriteUUID
*/
void PacketDecoder::decodeAlertData(const alertsVector& alerts) {
    
    frontStrengthVal = 0;
    rearStrengthVal = 0;

    std::vector<AlertTableData> alertDataList;

    for (int i = 0; i < alerts.size(); i++) {
        std::string alertIndexStr = alerts[i].substr(0, 2);
        std::string freqMSB = alerts[i].substr(2, 2);
        std::string freqLSB = alerts[i].substr(4, 2);

        int freqMhz = combineMSBLSB(freqMSB, freqLSB);
        freqGhz = static_cast<float>(freqMhz) / 1000.0f;

        std::string frontStrength = alerts[i].substr(6, 2);
        std::string rearStrength = alerts[i].substr(8, 2);
        std::string bandArrowDef = alerts[i].substr(10, 2);

        std::map<std::string, BandDirection> bandArrowMap;
            bandArrowMap["21"] = {"LASER", "FRONT"};
            bandArrowMap["81"] = {"LASER", "REAR"};
            bandArrowMap["24"] = {"K", "FRONT"};
            bandArrowMap["44"] = {"K", "SIDE"};
            bandArrowMap["84"] = {"K", "REAR"};
            bandArrowMap["22"] = {"KA", "FRONT"};
            bandArrowMap["42"] = {"KA", "SIDE"};
            bandArrowMap["82"] = {"KA", "REAR"};
            bandArrowMap["28"] = {"X", "FRONT"};
            bandArrowMap["48"] = {"X", "SIDE"};
            bandArrowMap["88"] = {"X", "REAR"};
            bandArrowMap["00"] = {"", ""};

        auto iBand = bandArrowMap.find(bandArrowDef);
        if (iBand != bandArrowMap.end()) {
            BandDirection direction = iBand->second;
            directionValue = direction.direction;
            bandValue = direction.band;
        } else {
            std::string errorMessage = "Error: Invalid key " + alerts[i] + " in bandArrowMap.";
            Serial.println(errorMessage.c_str());
        }

        std::string auxByte = alerts[i].substr(12, 2);
        if (auxByte == "80") {priority = true;}
        else if (auxByte == "40") {junkAlert = true;}
        else {priority = false; junkAlert = false;}

        std::map<std::string, alertByte> alertByteMap;
        // {alertCount, alertIndex}
        // Count: B0-B3, Index: B4-B7
        // bits 0-3 are for the # of alerts on the table. max of 15. bits 4-7 are for the alert index, again max of 15.
        // they are calculated individually rather than the bytesum
        alertByteMap["00"] = {0, 0};
        alertByteMap["11"] = {1, 1};
        alertByteMap["12"] = {2, 1};
        alertByteMap["13"] = {3, 1};
        alertByteMap["22"] = {2, 2};
        alertByteMap["23"] = {3, 2};
        alertByteMap["33"] = {3, 3};
        
        auto iCount = alertByteMap.find(alertIndexStr);
        if (iCount != alertByteMap.end()) {
            alertByte alerts = iCount->second;
            alertCountValue = alerts.count;
            alertIndexValue = alerts.index;
        } else {
            Serial.print("error in alertCountMap key: ");
            Serial.println(alertIndexStr.c_str());
        }

        bool found = false;
        for (auto& alertData : alertDataList) {
            if (alertData.alertCount == alertCountValue) {
                alertData.frequencies[alertData.freqCount++] = freqGhz;  // Store display-relevant data and increment counter
                found = true;
                break;
            }
        }

        if (!found) {
            AlertTableData newAlertData = {alertCountValue, {freqGhz}, {directionValue}, 1}; // Initialize the array
            alertDataList.push_back(newAlertData);
        }

        /* after this there should be no substring processing; we should only focus on painting the display */
        // paint the alert table arrows
        if (bandValue == "X") {
            frontStrengthVal = mapXToBars(frontStrength);
            rearStrengthVal = mapXToBars(rearStrength);
        } 
        else if (bandValue == "K") {
            frontStrengthVal = mapKToBars(frontStrength);
            rearStrengthVal = mapKToBars(rearStrength);
        }
        else if (bandValue == "KA") {
            frontStrengthVal = mapKaToBars(frontStrength);
            rearStrengthVal = mapKaToBars(rearStrength);        
        }
        // paint the priority alert
        if (priority && bandValue != "LASER") {
            // paint the main signal bar
            if (rearStrengthVal > frontStrengthVal) {
                set_var_prioBars(rearStrengthVal);
                //displayController.drawHorizontalBars(selectedConstants.MHZ_DISP_Y + (selectedConstants.MHZ_DISP_Y_OFFSET * i) - 10, rearStrengthVal, UI_COLOR); 
            }
            else {
                set_var_prioBars(frontStrengthVal);
                //displayController.drawHorizontalBars(selectedConstants.MHZ_DISP_Y + (selectedConstants.MHZ_DISP_Y_OFFSET * i) - 10, frontStrengthVal, UI_COLOR);
            }

            // paint the frequency of the prio alert
            if (freqGhz > 0) {
                char freqStr[16];
                snprintf(freqStr, sizeof(freqStr), "%.3f", freqGhz);
                set_var_prio_alert_freq(freqStr); 
            }
        }

        // enable below for debugging
        std::string decodedPayload = "INDX:" + std::to_string(alertIndexValue) +
                                    " FREQ_GHZ:" + std::to_string(freqGhz) +
                                    " FSTR:" + std::to_string(frontStrengthVal) +
                                    " RSTR:" + std::to_string(rearStrengthVal) +
                                    " BAND:" + bandValue +
                                    " BDIR:" + directionValue +
                                    " PRIO:" + std::to_string(priority) +
                                    " JUNK:" + std::to_string(junkAlert) +
                                    " SPEED: " + std::to_string(gpsData.speed);
                                    //+ " decode(ms): ";
        Serial.println(("respAlertData: " + decodedPayload).c_str());

    }
    set_var_alertCount(alertCountValue);
    set_var_frequencies(alertDataList);
}

/*  decode operation, passed from loop() - based on the packet ID.
    ID 31 (infDisplayData): route to decodeDisplayData
    ID 43 (respAlertData): direct decode 
*/
std::string PacketDecoder::decode(int lowSpeedThreshold, int currentSpeed) {
    unsigned long startTimeMillis = millis();

    std::string sof = packet.substr(0, 2);
    if (sof != "AA") {
        return "err SOF";
    }

    std::string endOfFrame = packet.substr(packet.length() - 2);
    if (endOfFrame != "AB") {
        return "err EOF";
    }

    std::string packetID = packet.substr(6, 2);

    if (packetID == "31") {
        /* infDisplayData - this should draw the main arrow(s) instead of the respAlertData 
           this will allow the incorporation of blinking arrow for prio alert on multiple alerts
        */
       std::string payload = packet.substr(12, 10);
       if (settings.displayTest || payload != lastinfPayload) {

        std::string bandArrow1 = packet.substr(16, 2);
        std::string bandArrow2 = packet.substr(18, 2);
        if (!bandArrow1.empty() && !bandArrow2.empty()) {
            BandArrowData arrow1Data = processBandArrow(bandArrow1);
            BandArrowData arrow2Data = processBandArrow(bandArrow2);

            compareBandArrows(arrow1Data, arrow2Data);
        }
        else {
            Serial.println("clearing alerts via ID31 bandArrow empty");
            clearInfAlerts();
        }

        std::string auxByte0 = packet.substr(packet.length() - 10, 2);
        std::string auxByte1 = packet.substr(packet.length() - 8, 2);
        std::string auxByte2 = packet.substr(packet.length() - 6, 2);
        if (!auxByte1.empty() && globalConfig.mode == nullptr) {
            int auxByte1Int = std::stoi(auxByte1, nullptr, 16);

            int modeBit0 = (auxByte1Int & 0b00000100) ? 1 : 0;
            int modeBit1 = (auxByte1Int & 0b00001000) ? 2 : 0;
            int mode = modeBit0 + modeBit1;

            switch(mode) {
                case 0:
                    globalConfig.mode = "Invalid Mode";
                    break;
                case 1:
                    globalConfig.mode = "ALL BOGEYS";
                    break;
                case 2:
                    globalConfig.mode = "LOGIC";
                    break;
                case 3:
                    globalConfig.mode = "ADV LOGIC";
                    break;
            }
        }

        // unsigned long elapsedTimeMillis = millis() - startTimeMillis;
        // Serial.print("infDisplayData loop time: ");
        // Serial.println(elapsedTimeMillis);
        }
        
        lastinfPayload = payload;
    }
    else if (packetID == "43") {
        /* respAlertData - should only be responsible for the alert table and priority alert frequency */
        std::string payload = packet.substr(10, packet.length() - 12);
        std::string alertC = payload.substr(0, 2);
        
        if (payload == lastPayload && alertC == "00") {
            // this shouldn't be necessary if inf is clearing the table anyway?
            if (alertPresent) {
                Serial.println("duplicate empty payload with alertPresent TRUE; setting to false");
                clearTableAlerts();
            }
            return "";
        } 
        else {
            // if (alertC == "00") {
            //     //Serial.println("empty ID43, clearing alerts");
            //     clearTableAlerts();
            //     return "";
            // }
            if (!muted) {
                if (currentSpeed <= lowSpeedThreshold) {
                    Serial.println("SilentRide requesting mute");
                    requestMute();
                }
            }
            lastPayload = payload;
            std::string alertIndexStr = packet.substr(10, 2);

            std::map<std::string, alertByte> alertByteMap;
            // {alertCount, alertIndex}
            // Count: B0-B3, Index: B4-B7; binary for the four bits for each var
            alertByteMap["00"] = {0, 0};
            alertByteMap["11"] = {1, 1};
            alertByteMap["12"] = {2, 1};
            alertByteMap["13"] = {3, 1};
            alertByteMap["22"] = {2, 2};
            alertByteMap["23"] = {3, 2};
            alertByteMap["33"] = {3, 3};
            
            auto iCount = alertByteMap.find(alertIndexStr);
            if (iCount != alertByteMap.end()) {
                alertByte alerts = iCount->second;
                alertCountValue = alerts.count;
                alertIndexValue = alerts.index;
            } else {
                Serial.print("error in alertCountMap key: ");
                Serial.println(alertIndexStr.c_str());
            }

            alertTable.push_back(payload);
            // check if the alertTable vector size is more than or equal to the tableSize (alerts.count) extracted from alertByte
            if (alertTable.size() >= alertCountValue) {
                alertPresent = true;
                decodeAlertData(alertTable);
                alertTable.clear();
            } else {
                if (alertCountValue == 0) {
                    // this doesn't ever seem to trigger
                    Serial.println("caught alertCountValue of 0 in ID43");
                    alertPresent = false;
                }
                // is there anything to be done here?
            }
            
        // unsigned long elapsedTimeMillis = millis() - startTimeMillis;
        // Serial.print("respAlertData loop time: ");
        // Serial.println(elapsedTimeMillis);
        }
    }
    else if (packetID == "12") {
        std::string userByteZero = packet.substr(10, 2);
        std::string userByteOne = packet.substr(12, 2);
        std::string userByteTwo = packet.substr(14, 2);

        decodeByteZero(userByteZero);
        decodeByteOne(userByteOne);
        decodeByteTwo(userByteTwo);
    }
    else if (packetID == "63") {
        Serial.println("respBatteryVoltage");
    }
    else if (packetID == "64") {
        Serial.println("respUnsupportedPacket");
    }
    else if (packetID == "66") {
        Serial.println("infV1Busy");
    }
    else if (packetID == "67") {
        Serial.println("respDataError encountered");
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
    
    // this is broken and needs fixing, but maybe not?
    /* Serial.print("Packet sent: ");
    for (int i = 0; i <= sizeof(packet + 2); i++) {
        Serial.print(packet[i], HEX);
    }
    Serial.println(); */
    return packet;
}

// TODO: add a conditional for whether the user wants fully blank display or BT light to illuminate
uint8_t* Packet::reqTurnOffMainDisplay() {
    uint8_t payloadData[] = {0x01, 0x01};
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
    Serial.println("Sending reqVersion packet");   
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQVERSION, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqSweepSections() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    Serial.println("Sending reqSweepSections packet");
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQSWEEPSECTIONS, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqSerialNumber() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    Serial.println("Sending reqSerialNumber packet");
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
    Serial.println("Sending reqMuteOff packet");
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQMUTEOFF, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqBatteryVoltage() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    Serial.println("Sending reqBatteryVoltage packet");
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQBATTERYVOLTAGE, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqCurrentVolume() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    Serial.println("Sending reqCurrentVolume packet");
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQCURRENTVOLUME, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}

uint8_t* Packet::reqUserBytes() {
    uint8_t payloadData[] = {0x01};
    uint8_t payloadLength = sizeof(payloadData) / sizeof(payloadData[0]);
    Serial.println("Sending reqUserBytes packet");
    return constructPacket(DEST_V1, REMOTE_SENDER, PACKET_ID_REQUSERBYTES, const_cast<uint8_t*>(payloadData), payloadLength, packet);
}