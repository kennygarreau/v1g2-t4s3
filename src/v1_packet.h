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
#define REMOTE_SENDER 0x06 // originate packets from 0x04 - "third party use"
#define PACKET_ID_REQVERSION 0x01
#define PACKET_ID_REQSERIALNUMBER 0x03
#define PACKET_ID_REQUSERBYTES 0x11
#define PACKET_ID_REQALLSWEEPDEFINITIONS 0x16
#define PACKET_ID_REQMAXSWEEPINDEX 0x19
#define PACKET_ID_REQSWEEPSECTIONS 0x22
#define PACKET_ID_REQTURNOFFMAINDISPLAY 0x32
#define PACKET_ID_REQTURNONMAINDISPLAY 0x33
#define PACKET_ID_REQMUTEON 0x34
#define PACKET_ID_REQMUTEOFF 0x35
#define PACKET_ID_REQCHANGEMODE 0x36
#define PACKET_ID_REQCURRENTVOLUME 0x37
#define PACKET_ID_REQSTARTALERTDATA 0x41
#define PACKET_ID_REQBATTERYVOLTAGE 0x62
#define PACKET_ID_REQSAVVYSTATUS 0x71
#define PACKET_ID_REQVEHICLESPEED 0x73
#define BAND_TIMEOUT_MS 500

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

struct BandDirection {
    const char* band;
    const char* direction; };

struct alertByte {
    int count;
    int index; };

enum Direction {
    DIR_NONE = 0,
    DIR_FRONT = 1,
    DIR_SIDE  = 2,
    DIR_REAR  = 3
};

struct BandState {
    bool active;
    uint32_t last_seen_ms;
};

enum Band {
    BAND_NONE = 0,
    BAND_LASER = 1,
    BAND_KA = 2,
    BAND_KU = 3,
    BAND_K = 4,
    BAND_X = 5
};

enum Modes {
    MODE_ALL_BOGEYS = 1,
    MODE_LOGIC = 2,
    MODE_ADVLOGIC = 3
};

enum class PhotoRadarType : uint8_t {
    None            = 0,
    MRCT            = 1,
    DriveSafeType1  = 2,
    DriveSafeType2  = 3,
    RedflexHalo     = 4,
    RedflexNK7      = 5,
    Ekin            = 6,
    Unknown         = 0xFF
};

static constexpr const char* PhotoRadarTypeNames[] = {
    "",
    "     MRCT",
    "DriveSafe Type 1",
    "DriveSafe Type 2",
    "  Redflex Halo",
    "  Redflex NK7",
    "     Ekin"
};

extern bool photoAlertPresent;
extern BandState ka_state;
extern BandState k_state;
extern BandState x_state;
extern BandState laser_state;

extern BandState front_state;
extern BandState side_state;
extern BandState rear_state;

void updateBandActivity(bool ka, bool k, bool x, bool laser);
void updateArrowActivity(bool front, bool side, bool rear);
void checkBandTimeouts();

using alertsVectorRaw = std::vector<std::vector<uint8_t>>;
extern std::vector<LogEntry> logHistory;

class PacketDecoder {
private:
    const std::vector<uint8_t>& rawpacket;
public:
    PacketDecoder(const std::vector<uint8_t>& rawpacket);

    std::string decode_v2(int lowSpeedThreshold, uint8_t currentSpeed);
    void decodeAlertData_v2(const alertsVectorRaw& alerts, int lowSpeedThreshold, uint8_t currentSpeed);
};

class Packet {
public:
    static uint8_t calculateChecksum(const uint8_t *data, size_t length);
    static uint8_t* constructPacket(uint8_t destID, uint8_t sendID, uint8_t packetID, uint8_t *payloadData, uint8_t payloadLength, uint8_t *packet);
    static uint8_t* reqStartAlertData();
    static uint8_t* reqVersion();
    static uint8_t* reqAllSweepDefinitions();
    static uint8_t* reqSweepSections();
    static uint8_t* reqMaxSweepIndex();
    static uint8_t* reqSerialNumber();
    static uint8_t* reqTurnOffMainDisplay(uint8_t mode);
    static uint8_t* reqTurnOnMainDisplay();
    static uint8_t* reqBatteryVoltage();
    static uint8_t* reqMuteOff();
    static uint8_t* reqMuteOn();
    static uint8_t* reqChangeMode(uint8_t mode);
    static uint8_t* reqCurrentVolume();
    static uint8_t* reqUserBytes();
    static uint8_t* reqSavvyStatus();
    static uint8_t* reqVehicleSpeed();
};

#endif // PACKETDECODER_H