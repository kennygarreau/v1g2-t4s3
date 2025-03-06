#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *logo_screen;
    lv_obj_t *main;
    lv_obj_t *settings;
    lv_obj_t *v1gen2logo;
    lv_obj_t *prioalertfreq;
    lv_obj_t *bt_logo;
    lv_obj_t *mode_type;
    lv_obj_t *mute_logo;
    lv_obj_t *alert_table;
    lv_obj_t *alert1freq;
    lv_obj_t *alert2freq;
    lv_obj_t *alert3freq;
    lv_obj_t *bar_str;
    lv_obj_t *nav_logo_enabled;
    lv_obj_t *nav_logo_disabled;
    lv_obj_t *band_ka;
    lv_obj_t *automutespeed;
    lv_obj_t *vehiclespeed;
    lv_obj_t *band_laser;
    lv_obj_t *band_k;
    lv_obj_t *band_x;
    //lv_obj_t *obj0;
    lv_obj_t *wifi_logo;
    lv_obj_t *wifi_local_logo;
    lv_obj_t *front_arrow;
    lv_obj_t *side_arrow;
    lv_obj_t *rear_arrow;
    lv_obj_t *label_ip;
    lv_obj_t *label_ip_val;
    lv_obj_t *label_ssid;
    lv_obj_t *label_ssid_val;
    lv_obj_t *label_pass;
    lv_obj_t *label_pass_val;
    lv_obj_t *slider_brightness;
    lv_obj_t *label_brightness;
    lv_obj_t *label_wifi;
    lv_obj_t *switch_wifi;
    lv_obj_t *label_v1cle;
    lv_obj_t *switch_v1cle;
    lv_obj_t *label_ble_rssi;
    lv_obj_t *label_ble_rssi_val;
    lv_obj_t *label_wifi_rssi_val;
    lv_obj_t *label_wifi_rssi;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_LOGO_SCREEN = 1,
    SCREEN_ID_MAIN = 2,
    SCREEN_ID_SETTINGS = 3,
};

enum BlinkIndices {
    BLINK_FRONT = 0,
    BLINK_SIDE = 1,
    BLINK_REAR = 2,
    BLINK_X = 3,
    BLINK_K = 4,
    BLINK_KA = 5,
    BLINK_LASER = 6
};

void create_screen_logo_screen();
void tick_screen_logo_screen();

void create_screen_main();
void tick_screen_main();

void create_screen_settings();
void tick_screen_settings();

void create_screens();
void tick_screen(int screen_index);

void tick_status_bar();

void update_alert_rows(int num_visible, const char* frequencies[]);
void register_blinking_image(int index, lv_obj_t *obj);
void init_blinking_system(void);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/