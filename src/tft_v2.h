#ifndef TFT_WRAPPER_H
#define TFT_WRAPPER_H

#ifdef __cplusplus

#include <vector>

extern "C" {
#endif

#include "lvgl.h"

void set_var_bt_connected(bool value);
bool get_var_muted();
void set_var_muted(bool value);
bool get_var_gpsEnabled();
const char *get_var_lowspeedthreshold();
const char *get_var_ssid();
const char *get_var_password();
const char *get_var_ipAddress();
bool get_var_wifiConnected();
bool get_var_wifiEnabled();
void set_var_wifiEnabled(bool enable);
void set_var_brightness(uint8_t value);
uint8_t get_var_brightness();

void set_var_prioBars(int value);
int get_var_prioBars();
void set_var_prio_alert_freq(const char *value);
bool get_showAlertTable();
void set_var_showAlertTable(bool value);
void set_var_alertCount(int value);
int get_var_alertCount();
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
