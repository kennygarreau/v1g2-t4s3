#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

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

// Native global variables

extern const char *get_var_prio_alert_freq();
extern void set_var_prio_alert_freq(const char *value);
extern const char *get_var_lowspeedthreshold();
extern const char *get_var_logicmode();
extern bool get_var_bt_connected();
extern void set_var_bt_connected(bool value);
// extern const char *get_var_logic_mode();
// extern void set_var_logic_mode(const char *value);
// extern const char *get_var_alert1freq();
// extern void set_var_alert1freq(const char *value);
// extern const char *get_var_alert2freq();
// extern void set_var_alert2freq(const char *value);
// extern const char *get_var_alert3freq();
// extern void set_var_alert3freq(const char *value);
//extern bool get_var_prio_bar1();
//extern void set_var_prio_bar1(bool value);
// extern bool get_var_gps_enabled();
// extern void set_var_gps_enabled(bool value);
// extern bool get_var_is_muted();
// extern void set_var_is_muted(bool value);
//extern void set_var_lowspeedthreshold(int32_t value);
// extern void set_var_logicmode(const char *value);
// extern bool get_var_laser_alert();
// extern void set_var_laser_alert(bool value);
// extern bool get_var_ka_alert();
// extern void set_var_ka_alert(bool value);
// extern bool get_var_k_alert();
// extern void set_var_k_alert(bool value);
// extern bool get_var_x_alert();
// extern void set_var_x_alert(bool value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/