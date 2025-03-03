#ifndef PACKETDECODER_H
#define PACKETDECODER_H

#include <string>
#include <vector>
#include "v1_config.h"

// Packet config
#define PACKETSTART 0xAA
#define PACKETEND 0xAB
#define REQVERSION 0x01
#define DEST_V1 0x0A // send packets to the v1 device id
//#define DEST_V1_LE 0x06
#define REMOTE_SENDER 0x04 // originate packets from 0x04 - "third party use"
#define PACKET_ID_REQVERSION 0x01
#define PACKET_ID_REQSERIALNUMBER 0x03
#define PACKET_ID_REQUSERBYTES 0x11
#define PACKET_ID_REQSWEEPSECTIONS 0x22
#define PACKET_ID_REQTURNOFFMAINDISPLAY 0x32
#define PACKET_ID_REQTURNONMAINDISPLAY 0x33
#define PACKET_ID_REQMUTEON 0x34
#define PACKET_ID_REQMUTEOFF 0x35
#define PACKET_ID_REQCURRENTVOLUME 0x37
#define PACKET_ID_REQSTARTALERTDATA 0x41
#define PACKET_ID_REQBATTERYVOLTAGE 0x62

struct BandArrowData {
    bool laser;
    bool ka;
    bool k;
    bool x;
    bool muteIndicator;
    bool front;
    bool side;
    bool rear;
};

using alertsVector = std::vector<std::string>;

class PacketDecoder {
private:
    std::string packet;
public:
    PacketDecoder(const std::string& packet);

    std::string decode(int lowSpeedThreshold, int currentSpeed);
    void decodeAlertData(const alertsVector& alerts, int lowSpeedThreshold, int curentSpeed);
    void clearInfAlerts();
    void clearTableAlerts();
};

class Packet {
public:
    static uint8_t calculateChecksum(const uint8_t *data, size_t length);
    static uint8_t* constructPacket(uint8_t destID, uint8_t sendID, uint8_t packetID, uint8_t *payloadData, uint8_t payloadLength, uint8_t *packet);
    static uint8_t* reqStartAlertData();
    static uint8_t* reqVersion();
    static uint8_t* reqSweepSections();
    static uint8_t* reqSerialNumber();
    static uint8_t* reqTurnOffMainDisplay(uint8_t mode);
    static uint8_t* reqTurnOnMainDisplay();
    static uint8_t* reqBatteryVoltage();
    static uint8_t* reqMuteOff();
    static uint8_t* reqMuteOn();
    static uint8_t* reqCurrentVolume();
    static uint8_t* reqUserBytes();
};

#endif // PACKETDECODER_H