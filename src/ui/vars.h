#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables
/*
enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_PRIO_ALERT_FREQ = 0,
    FLOW_GLOBAL_VARIABLE_LOGIC_MODE = 1,
    FLOW_GLOBAL_VARIABLE_ALERT1FREQ = 2,
    FLOW_GLOBAL_VARIABLE_ALERT2FREQ = 3,
    FLOW_GLOBAL_VARIABLE_ALERT3FREQ = 4,
    FLOW_GLOBAL_VARIABLE_BT_CONNECTED = 5,
    FLOW_GLOBAL_VARIABLE_PRIO_BAR1 = 6,
    FLOW_GLOBAL_VARIABLE_GPS_ENABLED = 7,
    FLOW_GLOBAL_VARIABLE_IS_MUTED = 8,
    FLOW_GLOBAL_VARIABLE_LOWSPEEDTHRESHOLD = 9,
    FLOW_GLOBAL_VARIABLE_LOGICMODE = 10,
    FLOW_GLOBAL_VARIABLE_LASER_ALERT = 11,
    FLOW_GLOBAL_VARIABLE_KA_ALERT = 12,
    FLOW_GLOBAL_VARIABLE_K_ALERT = 13,
    FLOW_GLOBAL_VARIABLE_X_ALERT = 14
};
*/

// Native global variables

extern const char *get_var_prio_alert_freq();
extern void set_var_prio_alert_freq(const char *value);
extern bool get_var_bt_connected();
//extern const char *get_var_lowspeedthreshold();
//extern const char *get_var_logicmode();
//extern void set_var_bt_connected(bool value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/