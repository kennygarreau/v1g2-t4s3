#ifndef TFT_WRAPPER_H
#define TFT_WRAPPER_H

#ifdef __cplusplus

#include <vector>

extern "C" {
#endif

#include "lvgl.h"

extern uint8_t activeBands;
void displayTestTask(void *pvParameters);

//double haversineDistance(double lat1, double lon1, double lat2, double lon2);
void main_press_handler(lv_event_t * e);
bool get_var_proxyConnected();
bool get_var_customFreqEnabled();
void set_var_bt_connected(bool value);
bool get_var_muted();
void set_var_muted(bool value);
bool get_var_gpsEnabled();
bool get_var_gpsAvailable();
const char *get_var_lowspeedthreshold();
const char *get_var_ssid();
const char *get_var_password();
const char *get_var_ipAddress();
bool get_var_usev1cle();
void set_var_usev1cle(bool switch_state);
bool get_var_v1clePresent();
bool get_var_wifiConnected();
bool get_var_wifiEnabled();
void set_var_wifiEnabled(bool enable);
bool get_var_localWifi();
void set_var_brightness(uint8_t value);
uint8_t get_var_brightness();
int getBluetoothSignalStrength();
int getWifiRSSI();
extern unsigned long getMillis();
void disconnectCurrentDevice();
const char *get_var_logicmode(bool value);
bool get_var_wifiClientConnected();

bool get_var_muteToGray();
void set_var_muteToGray(bool value);

bool get_var_colorBars();
void set_var_colorBars(bool value);

bool get_var_useDefaultV1Mode();
void set_var_useDefaultV1Mode(bool value);

bool get_var_showBogeys();
void set_var_showBogeys(bool value);

bool get_var_blankDisplay();
void set_var_blankDisplay(bool value);

bool get_var_dispBTIcon();
void set_var_dispBTIcon(bool value);

bool get_var_useImperial();
void set_var_useImperial(bool value);

int get_var_speedThreshold();
void set_var_speedThreshold(int value);

void set_var_prioBars(int value);
int get_var_prioBars();
void set_var_prio_alert_freq(const char *value);
bool get_showAlertTable();
void set_var_showAlertTable(bool value);
void set_var_alertCount(int value);
uint8_t get_var_alertCount();
void set_var_alertTableSize(int value);
int get_var_alertTableSize();
const char** get_var_frequencies();
const char** get_var_directions();

bool get_var_kAlert();
void set_var_kAlert(bool value);
bool get_var_kaAlert();
void set_var_kaAlert(bool value);
bool get_var_xAlert();
void set_var_xAlert(bool value);
bool get_var_laserAlert();
void set_var_laserAlert(bool value);
bool get_var_alertPresent();

void set_var_arrowPrioFront(bool value);
bool get_var_arrowPrioFront();
void set_var_arrowPrioSide(bool value);
bool get_var_arrowPrioSide();
void set_var_arrowPrioRear(bool value);
bool get_var_arrowPrioRear();

//void update_alert_rows(const char* alerts[], int num_alerts);

//pins
#define PIN_SD_CMD                   13
#define PIN_SD_CLK                   11
#define PIN_SD_D0                    12
#define PIN_D9   44  //              RXD0
#define PIN_D10  43  //              TXD0
#define RXD 44
#define TXD 43
#define PIN_IIC_SCL                  12
#define PIN_IIC_SDA                  11

#ifdef __cplusplus
}
void set_var_frequencies(const std::vector<AlertTableData>& alertDataList);

#endif

#endif // TFT_WRAPPER_H
