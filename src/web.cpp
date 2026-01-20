#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "v1_config.h"
#include "v1_packet.h"
#include "web.h"
#include "tft_v2.h"
#include "ble.h"
#include "ui/actions.h"
#include "ui/ui.h"

const char *lockoutFieldNames[] = {
    "act",
    "type",
    "ts",
    "last",
    "cnt",
    "lat",
    "lon",
    "spd",
    "course",
    "str",
    "dir",
    "freq"
};

const char* logFieldNames[] = {
    "ts", "lat", "long", "speed", "course", "str", "dir", "freq"
};

LockoutEntry savedLockoutLocations[] = {
    {
        .latitude = 37.7749,
        .longitude = -122.4194,
        .timestamp = 1710700000,
        .lastSeen = 1710705000,
        .speed = 45,
        .course = 270,
        .strength = 75,
        .direction = false, // Front
        .frequency = 24125,
        .counter = 3,
        .active = true,
        .entryType = false // Auto
    },
    {
        .latitude = 34.0522,
        .longitude = -118.2437,
        .timestamp = 1710600000,
        .lastSeen = 1710650000,
        .speed = 60,
        .course = 90,
        .strength = 80,
        .direction = true, // Rear
        .frequency = 24150,
        .counter = 5,
        .active = false,
        .entryType = true // Manual
    }
};

unsigned long rebootTime = 0;
bool isRebootPending, usingBattery;
float batteryPercentage = 0.0f;
float voltageInMv = 0.0f;
uint16_t vBusVoltage = 0;

std::string manufacturerName, modelNumber, serialNumber, softwareRevision;

void bootDeviceStats() {
    stats.totalHeap = ESP.getHeapSize();
    stats.totalPsram = ESP.getPsramSize();
    stats.cpuCores = ESP.getChipCores();
    stats.boardType = ESP.getChipModel();
    stats.boardRev = ESP.getChipRevision();
}

void getDeviceStats() {
    stats.uptime = (millis() - bootMillis) / 1000;

    if (WiFi.getMode() == WIFI_MODE_AP && WiFi.softAPgetStationNum() == 0) {
      stats.connectedWifiClients = 0;
    } else {
      stats.connectedWifiClients = WiFi.softAPgetStationNum();
    }
    
    isVBusIn = amoled.isVbusIn();
    if (isVBusIn) {
      vBusVoltage = amoled.getVbusVoltage();
    }
    if (bt_connected) {
      stats.btStr = getBluetoothSignalStrength();
    }
    
    if (amoled.getBusStatusString() == "No input") {
        usingBattery = true;
    } else {
        usingBattery = false;
    }

    if (usingBattery) {
        batteryCharging = amoled.isCharging();
        uint16_t espVoltage = amoled.getBattVoltage();
        voltageInMv = espVoltage; // cast this to float
        batteryPercentage = ((voltageInMv - EMPTY_VOLTAGE) / (FULLY_CHARGED_VOLTAGE - EMPTY_VOLTAGE)) * 100.0;
        batteryPercentage = constrain(batteryPercentage, 0, 100);
    }

    uint32_t cpuIdle = lv_timer_get_idle();
    stats.cpuBusy = 100 - cpuIdle;
    stats.freePsram = ESP.getFreePsram();
    stats.freeHeap = ESP.getFreeHeap();
    size_t largestBlock = ESP.getMaxAllocHeap();
    stats.heapFrag = (stats.freeHeap > 0) ? (100 - (largestBlock * 100 / stats.freeHeap)) : 0;
}

String readFileFromSPIFFS(const char *path)
{
    File file = SPIFFS.open(path, "r");
    if (!file || file.isDirectory())
    {
        return "<h1>404 - Page not found</h1>";
    }
    String content;
    while (file.available())
    {
        content += String((char)file.read());
    }
    file.close();
    return content;
}

void serveStaticFile(AsyncWebServer &server, const char *path, const char *mimeType)
{
    server.on(path, HTTP_GET, [path, mimeType](AsyncWebServerRequest *request) { 
        request->send(SPIFFS, path, mimeType); 
    });
}

void checkReboot() {
    if (isRebootPending && millis() - rebootTime >= 3000) {
        Serial.println("Rebooting...");
        show_popup("Rebooting...");
        ui_tick();
        lv_task_handler();
        delay(3000);
        ESP.restart();
    }
}

uint32_t hexToUint32(const String &hex) {
    String cleanHex = hex.startsWith("#") ? hex.substring(1) : hex;

    if (cleanHex.length() != 6) {
        Serial.println("Error: Invalid hex length");
        return 0;
    }

    for (char c : cleanHex) {
        if (!isxdigit(c)) {
            Serial.println("Error: Invalid hex character");
            return 0;
        }
    }

    uint32_t colorValue = strtoul(cleanHex.c_str(), nullptr, 16);
    // Debugging output to verify the conversion
    // Serial.print("Converted value: ");
    // Serial.println(colorValue, HEX);

    return colorValue;
}

void setupWebServer()
{
    if (wifiConnected) {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { 
            request->send(200, "text/html", readFileFromSPIFFS("/index.html")); 
        });
        Serial.println("serving network connected webserver");
    } else {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(200, "text/html", readFileFromSPIFFS("/index2.html")); 
        });
        Serial.println("serving local copy of webserver");
    }

    server.on("/firmware", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"version\":\"" + String(FIRMWARE_VERSION) + "\"}");
    });

    serveStaticFile(server, "/index.js", "application/javascript");
    serveStaticFile(server, "/settings.js", "application/javascript");
    serveStaticFile(server, "/update.js", "application/javascript");
    serveStaticFile(server, "/lockouts.js", "application/javascript");
    serveStaticFile(server, "/status.js", "application/javascript");
    serveStaticFile(server, "/js/chart.js", "application/javascript");
    serveStaticFile(server, "/js/moment.js", "application/javascript");
    serveStaticFile(server, "/js/chartjs-adapter-moment.js", "application/javascript");
    serveStaticFile(server, "/index2.html", "text/html");
    serveStaticFile(server, "/index.html", "text/html");
    serveStaticFile(server, "/update.html", "text/html");
    serveStaticFile(server, "/settings.html", "text/html");
    serveStaticFile(server, "/lockouts.html", "text/html");
    serveStaticFile(server, "/status.html", "text/html");
    serveStaticFile(server, "/style.css", "text/css");
    serveStaticFile(server, "/favicon.ico", "image/x-icon");

    server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument jsonDoc;
        JsonArray logArray = jsonDoc.createNestedArray("logs");
    
        for (const auto &logItem : logHistory) {
            JsonObject sectionObj = logArray.createNestedObject();
    
            for (int i = 0; i < LOG_FIELD_COUNT; i++) {
                switch (i) {
                    case LogField::TS:   sectionObj[logFieldNames[i]] = logItem.timestamp; break;
                    case LogField::LAT:    sectionObj[logFieldNames[i]] = logItem.latitude; break;
                    case LogField::LON:   sectionObj[logFieldNames[i]] = logItem.longitude; break;
                    case LogField::SPD:       sectionObj[logFieldNames[i]] = logItem.speed; break;
                    case LogField::CRSE:      sectionObj[logFieldNames[i]] = logItem.course; break;
                    case LogField::STR:    sectionObj[logFieldNames[i]] = logItem.strength; break;
                    case LogField::DIR:   sectionObj[logFieldNames[i]] = logItem.direction; break;
                    case LogField::FREQ:   sectionObj[logFieldNames[i]] = logItem.frequency; break;
                }
            }
        }
    
        String response;
        serializeJson(jsonDoc, response);
        request->send(200, "application/json", response);
    });   

    server.on("/lockouts", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument jsonDoc;
        JsonArray lockoutArray = jsonDoc.createNestedArray("lockouts");
    
        for (const auto &lockout : savedLockoutLocations) {
            JsonObject sectionObj = lockoutArray.createNestedObject();
    
            for (int i = 0; i < LOCKOUT_FIELD_COUNT; i++) {
                switch (i) {
                    case LockoutField::ACTIVE:      sectionObj[lockoutFieldNames[i]] = lockout.active; break;
                    case LockoutField::ENTRY_TYPE:  sectionObj[lockoutFieldNames[i]] = lockout.entryType; break;
                    case LockoutField::TIMESTAMP:   sectionObj[lockoutFieldNames[i]] = lockout.timestamp; break;
                    case LockoutField::LAST_SEEN:   sectionObj[lockoutFieldNames[i]] = lockout.lastSeen; break;
                    case LockoutField::COUNTER:     sectionObj[lockoutFieldNames[i]] = lockout.counter; break;
                    case LockoutField::LATITUDE:    sectionObj[lockoutFieldNames[i]] = lockout.latitude; break;
                    case LockoutField::LONGITUDE:   sectionObj[lockoutFieldNames[i]] = lockout.longitude; break;
                    case LockoutField::SPEED:       sectionObj[lockoutFieldNames[i]] = lockout.speed; break;
                    case LockoutField::COURSE:      sectionObj[lockoutFieldNames[i]] = lockout.course; break;
                    case LockoutField::STRENGTH:    sectionObj[lockoutFieldNames[i]] = lockout.strength; break;
                    case LockoutField::DIRECTION:   sectionObj[lockoutFieldNames[i]] = lockout.direction; break;
                    case LockoutField::FREQUENCY:   sectionObj[lockoutFieldNames[i]] = lockout.frequency; break;
                }
            }
        }
    
        String response;
        serializeJson(jsonDoc, response);
        request->send(200, "application/json", response);
    });    

    server.on("/gps-info", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument jsonDoc;

        if (xSemaphoreTake(gpsDataMutex, portMAX_DELAY)) {
            jsonDoc["latitude"] = gpsData.latitude;
            jsonDoc["longitude"] = gpsData.longitude;
            jsonDoc["speed"] = gpsData.speed;
            jsonDoc["altitude"] = gpsData.altitude;
            jsonDoc["course"] = gpsData.course;
            jsonDoc["time"] = gpsData.time;
            jsonDoc["date"] = gpsData.date;
            jsonDoc["hdop"] = gpsData.hdop;
            jsonDoc["satelliteCount"] = gpsData.satelliteCount;
            jsonDoc["signalQuality"] = gpsData.signalQuality;
            jsonDoc["timezone"] = settings.timezone;

            xSemaphoreGive(gpsDataMutex);
        }

        String jsonResponse;
        serializeJson(jsonDoc, jsonResponse);
        request->send(200, "application/json", jsonResponse); });

    server.on("/lockout-settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument jsonDoc;

        jsonDoc["autoLockoutsEnabled"] = autoLockoutSettings.enable;
        jsonDoc["lockoutRadius"] = autoLockoutSettings.radius;
        jsonDoc["lockoutThreshold"] = autoLockoutSettings.minThreshold;
        jsonDoc["lockoutLearnTime"] = autoLockoutSettings.learningTime;
        jsonDoc["lockoutColor"] = autoLockoutSettings.lockoutColor;
        jsonDoc["setLockoutColor"] = autoLockoutSettings.setLockoutColor;
        jsonDoc["requiredAlertCount"] = autoLockoutSettings.requiredAlerts;
        jsonDoc["inactiveTime"] = autoLockoutSettings.inactiveTime;

        String jsonResponse;
        serializeJson(jsonDoc, jsonResponse);
        request->send(200, "application/json", jsonResponse); 
    });

    server.on("/stats", HTTP_GET, [](AsyncWebServerRequest *request) {
        int frequency = getCpuFrequencyMhz();
        stats.wifiRSSI = getWifiRSSI();
        JsonDocument jsonDoc;

        jsonDoc["uptime"] = stats.uptime;
        jsonDoc["boardType"] = stats.boardType;
        jsonDoc["boardRev"] = stats.boardRev;
        jsonDoc["frequency"] = frequency;
        jsonDoc["cpuBusy"] = stats.cpuBusy;
        jsonDoc["cpuCores"] = stats.cpuCores;
        jsonDoc["totalHeap"] = stats.totalHeap / 1024;
        jsonDoc["freeHeapInKB"] = stats.freeHeap / 1024;
        jsonDoc["heapFrag"] = stats.heapFrag;
        jsonDoc["totalPsram"] = stats.totalPsram / 1024;
        jsonDoc["freePsramInKB"] = stats.freePsram / 1024;
        jsonDoc["totalStorage"] = stats.totalStorageKB;
        jsonDoc["usedStorage"] = stats.usedStorageKB;
        jsonDoc["connectedWifiClients"] = stats.connectedWifiClients;
        jsonDoc["bluetoothRSSI"] = stats.btStr;
        jsonDoc["wifiRSSI"] = stats.wifiRSSI;
        jsonDoc["usingBattery"] = usingBattery;
        jsonDoc["batteryPercent"] = batteryPercentage;
        jsonDoc["espVoltage"] = voltageInMv;

        if (isVBusIn) {
            jsonDoc["vBusVoltage"] = vBusVoltage;
        }
        if (batteryCharging) {
            jsonDoc["batteryCharging"] = "(charging)";
        }
        jsonDoc["carVoltage"] = stats.voltage;

        String jsonResponse;
        serializeJson(jsonDoc, jsonResponse);
        request->send(200, "application/json", jsonResponse); 
    });

    server.on("/board-info", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument jsonDoc;

        jsonDoc["manufacturer"] = manufacturerName;
        jsonDoc["model"] = modelNumber;
        jsonDoc["serial"] = serialNumber;
        jsonDoc["softwareVersion"] = softwareRevision;

        // User Byte 0
        JsonObject configJson = jsonDoc.createNestedObject("config");
        configJson["Mode"] = globalConfig.mode;
        configJson["X Band"] = globalConfig.xBand ? "On" : "Off";
        configJson["K Band"] = globalConfig.kBand ? "On" : "Off";
        configJson["Ka Band"] = globalConfig.kaBand ? "On" : "Off";
        configJson["Laser"] = globalConfig.laserBand ? "On" : "Off";
        configJson["Mute to"] = globalConfig.muteTo;
        configJson["Bogey-Lock Loud after muting"] = globalConfig.bogeyLockLoud ? "On" : "Off";
        configJson["X and K Rear Mute in Logic Modes"] = globalConfig.rearMute ? "Off" : "On";
        configJson["Ku Band"] = globalConfig.kuBand ? "Off" : "On";

        // User Byte 1
        configJson["Euro Mode"] = globalConfig.euro ? "Off" : "On";
        configJson["K-Verifier"] = globalConfig.kVerifier ? "On" : "Off";
        configJson["Rear Laser"] = globalConfig.rearLaser ? "On" : "Off";
        configJson["Custom Frequencies"] = globalConfig.customFreqEnabled ? "Disabled" : "Enabled";
        configJson["Ka Always Priority Alert"] = globalConfig.kaAlwaysPrio ? "Off" : "On";
        configJson["Fast Laser Detection"] = globalConfig.fastLaserDetection ? "On" : "Off";
        configJson["Ka Sensitivity"] = globalConfig.kaSensitivity;

        // User Byte 2
        configJson["Startup Sequence"] = globalConfig.startupSequence ? "On" : "Off";
        configJson["Resting Display"] = globalConfig.restingDisplay ? "On" : "Off";
        configJson["BSM Plus"] = globalConfig.bsmPlus ? "Off" : "On";
        configJson["Auto Mute"] = globalConfig.autoMute;
        configJson["K Sensitivity"] = globalConfig.kSensitivity;

        // User Byte 3
        configJson["X Sensitivity"] = globalConfig.xSensitivity;
        configJson["MRCT Photo"] = globalConfig.mrctPhoto ? "Off" : "On";
        configJson["DriveSafe 3D Photo"] = globalConfig.driveSafe3dPhoto ? "Off" : "On";
        configJson["DriveSafe 3DHD Photo"] = globalConfig.driveSafe3dHdPhoto ? "Off" : "On";
        configJson["Redflex Halo Photo"] = globalConfig.redflexHaloPhoto ? "Off" : "On";
        configJson["Redflex NK7 Photo"] = globalConfig.redflexNK7Photo ? "Off" : "On";
        configJson["Ekin Photo"] = globalConfig.ekinPhoto ? "Off" : "On";
        configJson["Photo Verifier"] = globalConfig.photoVerifier ? "Off" : "On";
        
        configJson["Main Volume"] = globalConfig.mainVolume;
        configJson["Muted Volume"] = globalConfig.mutedVolume;

        JsonObject sweepSettingsJson = jsonDoc.createNestedObject("sweepSettings");
        // TODO: any use for this outside of the internal logic?
        //sweepSettingsJson["maxSweepIndex"] = globalConfig.maxSweepIndex;
        
        JsonArray sectionArray = sweepSettingsJson.createNestedArray("sweepSections");
        for (const auto& section : globalConfig.sections) {
            JsonObject sectionObj = sectionArray.createNestedObject();
            sectionObj["lowerBound"] = section.first;
            sectionObj["upperBound"] = section.second;
        }

        JsonArray sweepsArray = sweepSettingsJson.createNestedArray("customSweeps");
        for (const auto& section : globalConfig.sweeps) {
            JsonObject sectionObj = sweepsArray.createNestedObject();
            sectionObj["lowerBound"] = section.first;
            sectionObj["upperBound"] = section.second;
        }
        
        JsonObject displaySettingsJson = jsonDoc.createNestedObject("displaySettings");
        displaySettingsJson["brightness"] = settings.brightness;
        displaySettingsJson["wifiMode"] = settings.wifiMode;
        displaySettingsJson["localSSID"] = settings.localSSID;
        displaySettingsJson["localPW"] = settings.localPW;
        displaySettingsJson["disableBLE"] = settings.disableBLE;
        displaySettingsJson["proxyBLE"] = settings.proxyBLE;
        displaySettingsJson["useV1LE"] = settings.useV1LE;
        displaySettingsJson["timezone"] = settings.timezone;
        displaySettingsJson["enableGPS"] = settings.enableGPS;
        displaySettingsJson["enableWifi"] = settings.enableWifi;
        displaySettingsJson["lowSpeedThreshold"] = settings.lowSpeedThreshold;
        displaySettingsJson["displayOrientation"] = settings.displayOrientation;
        displaySettingsJson["isPortraitMode"] = settings.isPortraitMode;
        displaySettingsJson["useDefaultV1Mode"] = settings.useDefaultV1Mode;
        displaySettingsJson["turnOffDisplay"] = settings.turnOffDisplay;
        displaySettingsJson["onlyDisplayBTIcon"] = settings.onlyDisplayBTIcon;
        displaySettingsJson["displayTest"] = settings.displayTest;
        displaySettingsJson["unitSystem"] =
            (settings.unitSystem == IMPERIAL) ? "Imperial" : "Metric";
        displaySettingsJson["muteToGray"] = settings.muteToGray;
        displaySettingsJson["colorBars"] = settings.colorBars;
        displaySettingsJson["showBogeys"] = settings.showBogeyCount;

        JsonArray wifiArray = displaySettingsJson.createNestedArray("wifiCredentials");
        for (const auto& cred : settings.wifiCredentials) {
            JsonObject wifiObj = wifiArray.createNestedObject();
            wifiObj["ssid"] = cred.ssid;
            wifiObj["password"] = cred.password;
        }

        char hexColor[8];
        sprintf(hexColor, "#%06X", settings.textColor & 0xFFFFFF);
        //Serial.printf("Stored color value: 0x%06X\n", settings.textColor & 0xFFFFFF);
        displaySettingsJson["textColor"] = hexColor;

        String jsonResponse;
        serializeJson(jsonDoc, jsonResponse);
        request->send(200, "application/json", jsonResponse); 
    });

    server.on("/updateFirmware", HTTP_POST, [](AsyncWebServerRequest *request) {
        bool reboot = !Update.hasError();
        AsyncWebServerResponse* response = request->beginResponse(200, "text/plain",
            reboot ? "Update Success! Rebooting..." : "Update Failed!");
        response->addHeader("Connection", "close");
        request->send(response);
        if (reboot) {
            isRebootPending = true;
            rebootTime = millis();
            } }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                Serial.printf("Update Start: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }
            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }
            if (final) {
                if (Update.end(true)) {
                    Serial.printf("Update Success: %u bytes\n", index + len);
                } else {
                    Update.printError(Serial);
                }
            } 
    });

    server.on("/updateFilesystem", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Filesystem updated. Rebooting...");
        ESP.restart();
        }, [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (!index) {
            Serial.printf("Starting filesystem update: %s\n", filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
                Update.printError(Serial);
            }
        }
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
        if (final) {
            if (Update.end(true)) {
                Serial.printf("Filesystem update successful: %u bytes\n", index + len);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.on("/updateLockoutSettings", HTTP_POST, 
        [](AsyncWebServerRequest *request){},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        
            String body = String((char*)data).substring(0, len);

            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, body);

            if (error) {
                Serial.println("Failed to parse JSON");
                request->send(400, "application/json", "{\"error\": \"Failed to parse JSON\"}");
                return;
            }
            preferences.begin("lockoutSettings", false);

            Serial.println("Lockout settings updated:");
            if (doc.containsKey("autoLockoutsEnabled")) {
                autoLockoutSettings.enable = doc["autoLockoutsEnabled"].as<bool>();
                Serial.println("autoLockoutsEnabled: " + String(autoLockoutSettings.enable));
                preferences.putBool("enabled", autoLockoutSettings.enable);
            }
            if (doc.containsKey("setLockoutColor")) {
                autoLockoutSettings.setLockoutColor = doc["setLockoutColor"].as<bool>();
                Serial.println("setLockoutColor: " + String(autoLockoutSettings.setLockoutColor));
                preferences.putBool("setLockoutColor", autoLockoutSettings.setLockoutColor);
            }
            if (doc.containsKey("requiredAlertCount")) {
                autoLockoutSettings.requiredAlerts = doc["requiredAlertCount"].as<int>();
                Serial.println("requiredAlertCount: " + String(autoLockoutSettings.requiredAlerts));
                preferences.putInt("requiredAlerts", autoLockoutSettings.requiredAlerts);
            }
            if (doc.containsKey("lockoutLearnTime")) {
                autoLockoutSettings.learningTime = doc["lockoutLearnTime"].as<int>();
                Serial.println("lockoutLearnTime: " + String(autoLockoutSettings.learningTime));
                preferences.putInt("learningTime", autoLockoutSettings.learningTime);
            }
            if (doc.containsKey("lockoutThreshold")) {
                autoLockoutSettings.minThreshold = doc["lockoutThreshold"].as<int>();
                Serial.println("lockoutThreshold: " + String(autoLockoutSettings.minThreshold));
                preferences.putInt("minThreshold", autoLockoutSettings.minThreshold);
            }
            if (doc.containsKey("lockoutColor")) {
                autoLockoutSettings.lockoutColor = hexToUint32(doc["lockoutColor"].as<String>());
                Serial.println("lockoutColor: " + String(autoLockoutSettings.lockoutColor));
                preferences.putUInt("lockoutColor", autoLockoutSettings.lockoutColor);
            }
            preferences.end();

            request->send(200, "application/json", "{\"message\": \"Lockout settings updated successfully!\"}");
    });

    server.on("/updateSettings", HTTP_POST, 
        [](AsyncWebServerRequest *request){},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        
            String body = String((char*)data).substring(0, len);

            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, body);

            if (error) {
                Serial.println("Failed to parse JSON");
                request->send(400, "application/json", "{\"error\": \"Failed to parse JSON\"}");
                return;
            }
            preferences.begin("settings", false);

            Serial.println("Settings updated:");
            if (doc.containsKey("brightness")) {
                settings.brightness = doc["brightness"].as<u8_t>();
                Serial.println("brightness: " + String(settings.brightness));
                preferences.putUInt("brightness", settings.brightness);
                amoled.setBrightness(settings.brightness);
            }
            if (doc.containsKey("wifiMode")) {
                int mode = doc["wifiMode"].as<int>();
            
                if (mode >= WIFI_SETTING_STA && mode <= WIFI_SETTING_APSTA) {
                    if (settings.wifiMode != static_cast<WiFiModeSetting>(mode)) {

                        settings.wifiMode = static_cast<WiFiModeSetting>(mode);
                        preferences.putInt("wifiMode", mode);

                        Serial.printf("wifiMode changed → %d, reboot scheduled\n", mode);
                        isRebootPending = true;

                    } else {
                        Serial.println("wifiMode unchanged — no reboot");
                    }
                } else {
                    Serial.println("Invalid wifiMode received!");
                }
            }
            if (doc.containsKey("wifiCredentials") && doc["wifiCredentials"].is<JsonArray>()) {
                JsonArray wifiCredentials = doc["wifiCredentials"].as<JsonArray>();
                int networkCount = min((int)wifiCredentials.size(), MAX_WIFI_NETWORKS);
                
                Serial.println("Updating WiFi credentials...");
                
                for (int i = 0; i < networkCount; i++) {
                    JsonObject network = wifiCredentials[i];
                    if (network.containsKey("ssid") && network.containsKey("password")) {
                        String ssid = network["ssid"].as<String>();
                        String password = network["password"].as<String>();
            
                        Serial.println("Storing WiFi network " + String(i) + ": " + ssid);
                        preferences.putString(("wifi_ssid_" + String(i)).c_str(), ssid);
                        preferences.putString(("wifi_pass_" + String(i)).c_str(), password);
                    }
                }
                
                preferences.putInt("networkCount", networkCount);
                Serial.println("Total networks stored: " + String(networkCount));
            }
            if (doc.containsKey("localSSID")) {
                settings.localSSID = doc["ssid"].as<String>();
                Serial.println("ssid: " + settings.localSSID);
                preferences.putString("ssid", settings.localSSID);
            }
            if (doc.containsKey("localPW")) {
                settings.localPW = doc["password"].as<String>();
                Serial.println("password: " + settings.localPW);
                preferences.putString("password", settings.localPW);
            }
            if (doc.containsKey("disableBLE")) {
                settings.disableBLE = doc["disableBLE"].as<bool>();
                Serial.println("disableBLE: " + String(settings.disableBLE));
                preferences.putBool("disableBLE", settings.disableBLE);
            }
            if (doc.containsKey("proxyBLE")) {
                settings.proxyBLE = doc["proxyBLE"].as<bool>();
                Serial.println("proxyBLE: " + String(settings.proxyBLE));
                preferences.putBool("proxyBLE", settings.proxyBLE);
            }
            if (doc.containsKey("useV1LE")) {
                settings.useV1LE = doc["useV1LE"].as<bool>();
                Serial.println("useV1LE: " + String(settings.useV1LE));
                preferences.putBool("useV1LE", settings.useV1LE);
                disconnectCurrentDevice();
            }
            if (doc.containsKey("enableGPS")) {
                settings.enableGPS = doc["enableGPS"].as<bool>();
                Serial.println("enableGPS: " + String(settings.enableGPS));
                preferences.putBool("enableGPS", settings.enableGPS);
                isRebootPending = true;
            }
            if (doc.containsKey("lowSpeedThreshold")) {
                settings.lowSpeedThreshold = doc["lowSpeedThreshold"].as<int>();
                Serial.println("lowSpeedThreshold: " + String(settings.lowSpeedThreshold));
                preferences.putInt("lowSpeedThres", settings.lowSpeedThreshold);
            }
            if (doc.containsKey("displayOrientation")) {
                settings.displayOrientation = doc["displayOrientation"].as<int>();
                Serial.println("displayOrientation: " + String(settings.displayOrientation));
                preferences.putInt("displayOrient", settings.displayOrientation);
                //isRebootPending = true;
            }
            if (doc.containsKey("textColor")) {
                settings.textColor = hexToUint32(doc["textColor"].as<String>());
                Serial.println("textColor: " + String(settings.textColor));
                preferences.putUInt("textColor", settings.textColor);
            }
            if (doc.containsKey("useDefaultV1Mode")) {
                settings.useDefaultV1Mode = doc["useDefaultV1Mode"].as<bool>();
                Serial.println("useDefaultV1Mode: " + String(settings.useDefaultV1Mode));
                preferences.putBool("useDefMode", settings.useDefaultV1Mode);
            }
            if (doc.containsKey("displayTest")) {
                settings.displayTest = doc["displayTest"].as<bool>();
                Serial.println("displayTest: " + String(settings.displayTest));
                preferences.putBool("displayTest", settings.displayTest);
                //preferences.end();
                isRebootPending = true;
            }
            if (doc.containsKey("onlyDisplayBTIcon")) {
                settings.onlyDisplayBTIcon = doc["onlyDisplayBTIcon"].as<bool>();
                Serial.println("onlyDisplayBTIcon: " + String(settings.onlyDisplayBTIcon));
                preferences.putBool("onlyDispBTIcon", settings.onlyDisplayBTIcon);
                clientWriteCharacteristic->writeValue((uint8_t*)Packet::reqTurnOffMainDisplay(static_cast<uint8_t>(settings.onlyDisplayBTIcon)), 7, false);
            }
            if (doc.containsKey("turnOffDisplay")) {
                settings.turnOffDisplay = doc["turnOffDisplay"].as<bool>();
                Serial.println("turnOffDisplay: " + String(settings.turnOffDisplay));
                preferences.putBool("turnOffDisplay", settings.turnOffDisplay);
            }
            if (doc.containsKey("unitSystem")) {
                const char* uStr = doc["unitSystem"];
                if (strcmp(uStr, "Metric") == 0) {
                    settings.unitSystem = METRIC;
                } else {
                    settings.unitSystem = IMPERIAL;
                }
                Serial.println("unitSystem: " + settings.unitSystem);
                preferences.putString("unitSystem", uStr);
            }
            if (doc.containsKey("muteToGray")) {
                settings.muteToGray = doc["muteToGray"].as<bool>();
                Serial.println("muteToGray: " + String(settings.muteToGray));
                preferences.putBool("muteToGray", settings.muteToGray);
            }
            if (doc.containsKey("colorBars")) {
                settings.colorBars = doc["colorBars"].as<bool>();
                Serial.println("colorBars: " + String(settings.colorBars));
                preferences.putBool("colorBars", settings.colorBars);
            }
            if (doc.containsKey("showBogeys")) {
                settings.showBogeyCount = doc["showBogeys"].as<bool>();
                Serial.println("showBogeys: " + String(settings.showBogeyCount));
                preferences.putBool("showBogeys", settings.showBogeyCount);
            }
            if (doc.containsKey("timezone")) {
                settings.timezone = doc["timezone"].as<String>();
                Serial.println("timezone: " + settings.timezone);
                preferences.putString("timezone", settings.timezone);
            }
            preferences.end();


            request->send(200, "application/json", "{\"message\": \"Settings updated successfully!\"}");
    });

    server.begin();
    webStarted = true;
}