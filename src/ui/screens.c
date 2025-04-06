#include <string.h>
#include <stdio.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "tft_v2.h"

objects_t objects;
lv_obj_t *tick_value_change_obj;
lv_obj_t* signal_bars[MAX_BARS];
lv_obj_t* alert_rows[MAX_ALERT_ROWS];
lv_obj_t* alert_directions[MAX_ALERT_ROWS];

lv_obj_t *blink_images[MAX_BLINK_IMAGES];  // Store elements to blink
bool blink_enabled[MAX_BLINK_IMAGES] = {false}; // Track which element should blink
int blink_count = 0;
int cur_bars = 0;
//int cur_alert_count = 0;

static uint32_t last_blink_time = 0;
static bool blink_state = false;

uint32_t default_color = 0xffff0000;
uint32_t gray_color = 0xff636363;

uint32_t green_bar = 0xff8cd47e;
uint32_t yellow_bar = 0xfff8d66d;
uint32_t orange_bar = 0xffffb54c;

lv_obj_t* create_alert_row(lv_obj_t* parent, int x, int y, const char* frequency) {
    lv_obj_t* obj = lv_label_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_text(obj, frequency);
    lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    return obj;
}

lv_obj_t* create_alert_direction(lv_obj_t* parent, int x, int y) {
    lv_obj_t* img = lv_img_create(parent);
    lv_obj_set_pos(img, x, y);
    lv_img_set_src(img, &img_small_arrow_rear); // placeholder
    lv_obj_set_size(img, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    //lv_obj_set_size(img, 32, 32);
    lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    return img;
}

void create_alert_rows(lv_obj_t* parent, int num_rows) {
    int base_x_text = 45;       // Base X position for text
    int base_y_text = 2;      // Starting Y position
    int y_step_text = -52;       // Vertical spacing between rows

    int base_x_img = -4;
    int base_y_img = 6;
    int y_step_img = -48;

    for (int i = 0; i < num_rows && i < MAX_ALERT_ROWS; ++i) {
        alert_rows[i] = create_alert_row(parent, base_x_text, base_y_text - (i * y_step_text), "      ");
        alert_directions[i] = create_alert_direction(parent, base_x_img, base_y_img - (i * y_step_img));
    }
}

void update_alert_rows(int num_alerts, const char* frequencies[], bool muted, bool muteToGray) {
    if (frequencies == NULL) {
        return;
    }

    for (int i = 0; i < MAX_ALERT_ROWS; i++) {
        if (i < num_alerts) {
            if (muted && muteToGray) {
                lv_obj_set_style_text_color(alert_rows[i], lv_color_hex(gray_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            } 
            else {
                lv_obj_set_style_text_color(alert_rows[i], lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            lv_label_set_text(alert_rows[i], frequencies[i]);
            lv_obj_clear_flag(alert_rows[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(alert_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void update_alert_arrows(int num_alerts, const char* directions[], bool muted, bool muteToGray) {
    if (directions == NULL) {
        return;
    }

    for (int i = 0; i < MAX_ALERT_ROWS; i++) {
        if (i < num_alerts && directions[i] != NULL) {
            if (strcmp(directions[i], "FRONT") == 0) {
                lv_img_set_src(alert_directions[i], &img_small_arrow_front);
            } else if (strcmp(directions[i], "SIDE") == 0) {
                lv_img_set_src(alert_directions[i], &img_small_arrow_side);
            } else if (strcmp(directions[i], "REAR") == 0) {
                lv_img_set_src(alert_directions[i], &img_small_arrow_rear);
            } else {
                lv_img_set_src(alert_directions[i], &img_small_arrow_front);
            }

            if (alert_directions[i] != NULL) {
                lv_obj_clear_flag(alert_directions[i], LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            if (alert_directions[i] != NULL) {
                lv_obj_add_flag(alert_directions[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

lv_obj_t* create_signal_bar(lv_obj_t* parent, int x, int y) {
    lv_obj_t* obj = lv_label_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    //lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, "             ");
    lv_obj_set_style_bg_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    return obj;
}

void create_signal_bars(lv_obj_t* parent, int num_bars) {
    int base_x = 76;       // Base X position for all bars
    int base_y = 264;      // Starting Y position
    int y_step = 30;       // Vertical spacing between bars

    for (int i = 0; i < num_bars && i < MAX_BARS; ++i) {
        signal_bars[i] = create_signal_bar(parent, base_x, base_y - (i * y_step));
    }
}

void update_signal_bars(int num_visible) {
    bool muted = get_var_muted();
    bool muteToGray = get_var_muteToGray();
    bool colorBars = get_var_colorBars();

    for (int i = 0; i < MAX_BARS; ++i) {
        if (signal_bars[i] == NULL) {
            continue;
        }

        if (i < num_visible) {
            uint32_t bar_color = default_color;

            if (muted && muteToGray) {
                bar_color = gray_color;
            } else if (colorBars) {
                bar_color = get_bar_color(i);
            }
            
            lv_obj_set_style_bg_color(signal_bars[i], lv_color_hex(bar_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_clear_flag(signal_bars[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(signal_bars[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void register_blinking_image(int index, lv_obj_t *obj) {
    if (index < 0 || index >= MAX_BLINK_IMAGES) return;
    blink_images[index] = obj;
}

static void blink_timer_cb(lv_timer_t *timer) {
    for (int i = 0; i < blink_count; i++) {
        if (blink_enabled[i]) {
            bool is_hidden = lv_obj_has_flag(blink_images[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(blink_images[i], is_hidden ? LV_OBJ_FLAG_HIDDEN : 0);
        } 
    }
}

void init_blinking_system() {
    lv_timer_create(blink_timer_cb, BLINK_FREQUENCY, NULL);
}

void create_screen_logo_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.logo_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 600, 450);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // v1gen2logo
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.v1gen2logo = obj;
            lv_obj_set_pos(obj, 112, 183);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_v1gen2logo_white);
        }
    }
}

void tick_screen_logo_screen() {
}

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 600, 450);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    {
        lv_obj_t *parent_obj = obj;
        {
            // prioalertfreq
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.prioalertfreq = obj;
            lv_obj_set_pos(obj, 120, 341);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_112, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // bt_logo
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.bt_logo = obj;
            lv_obj_set_pos(obj, 534, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_bt_logo_small);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // nav_logo_enabled
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.nav_logo_enabled = obj;
            lv_obj_set_pos(obj, 466, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_location_red);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // nav_logo_disabled
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.nav_logo_disabled = obj;
            lv_obj_set_pos(obj, 466, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_location_disabled);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // wifi
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.wifi_logo = obj;
            lv_obj_set_pos(obj, 502, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_wifi);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // wifi_local
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.wifi_local_logo = obj;
            lv_obj_set_pos(obj, 502, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_wifi_local);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // mute_logo
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.mute_logo = obj;
            lv_obj_set_pos(obj, 432, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_mute_logo_small);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // mode_type
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.mode_type = obj;
            lv_obj_set_pos(obj, 17, 16);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // default v1 mode
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.default_mode = obj;
            lv_obj_set_pos(obj, 17, 341);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_112, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

            // Create the black "3" overlay
            lv_obj_t *overlay = lv_label_create(parent_obj);
            objects.overlay_mode = overlay;
            lv_obj_set_pos(overlay, 17, 341); // Same position as default_mode
            lv_obj_set_size(overlay, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(overlay, "");
            lv_obj_set_style_text_font(overlay, &ui_font_alarmclock_112, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(overlay, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // custom freqs enabled
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.custom_freq_en = obj;
            lv_obj_set_pos(obj, 60, 341);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            //lv_label_set_text(obj, ".");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_112, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // alert_table
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.alert_table = obj;
            lv_obj_set_pos(obj, 398, 72);
            lv_obj_set_size(obj, 192, 240);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff212124), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

            create_alert_rows(obj, MAX_ALERT_ROWS);
        }
        {
            // main_signal_bars
            create_signal_bars(parent_obj, MAX_BARS);
        }
        {
            // front arrow
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.front_arrow = obj;
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_arrow_front);
            lv_img_set_src(obj, psram_img);
            lv_obj_align(obj, LV_ALIGN_CENTER, -24, -70);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // side arrow
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.side_arrow = obj;
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_arrow_side);
            lv_img_set_src(obj, psram_img);
            lv_obj_align(obj, LV_ALIGN_CENTER, -24, 0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // rear arrow
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.rear_arrow = obj;
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_arrow_rear);
            lv_img_set_src(obj, psram_img);
            lv_obj_align(obj, LV_ALIGN_CENTER, -24, 50);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        // {
        //     // vehiclespeed
        //     lv_obj_t *obj = lv_label_create(parent_obj);
        //     objects.vehiclespeed = obj;
        //     lv_obj_set_pos(obj, 32, 351);
        //     lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        //     lv_label_set_text(obj, "");
        //     lv_obj_set_style_text_font(obj, &ui_font_noto_speed_48, LV_PART_MAIN | LV_STATE_DEFAULT);
        //     lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        //     lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        // }
        // {
        //     // band_laser
        //     lv_obj_t *obj = lv_label_create(parent_obj);
        //     objects.band_laser = obj;
        //     lv_obj_set_pos(obj, 14, 108);
        //     lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        //     lv_label_set_text(obj, "L");
        //     lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
        //     lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        //     lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        // }
        {
            // band_ka
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.band_ka = obj;
            lv_obj_set_pos(obj, 14, 146);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "KA");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // band_k
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.band_k = obj;
            lv_obj_set_pos(obj, 14, 184);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "K");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // band_x
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.band_x = obj;
            lv_obj_set_pos(obj, 14, 222);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "X");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {   
            // initialize blinking callback and object mapping
            init_blinking_system();
            register_blinking_image(BLINK_FRONT, objects.front_arrow);
            register_blinking_image(BLINK_SIDE, objects.side_arrow);
            register_blinking_image(BLINK_REAR, objects.rear_arrow);
            register_blinking_image(BLINK_X, objects.band_x);
            register_blinking_image(BLINK_K, objects.band_k);
            register_blinking_image(BLINK_KA, objects.band_ka);
            register_blinking_image(BLINK_LASER, objects.band_laser);
        }
    }
}

void tick_status_bar() {
    bool laserAlert = get_var_laserAlert();
    bool gps_enabled = get_var_gpsEnabled();

    // Bluetooth status
    {
        bool bt_connected = get_var_bt_connected(); // true when connected
    
        lv_obj_clear_flag(objects.bt_logo, bt_connected ? LV_OBJ_FLAG_HIDDEN : 0);
        lv_obj_add_flag(objects.bt_logo, bt_connected ? 0 : LV_OBJ_FLAG_HIDDEN);
        LV_LOG_INFO("Updated Bluetooth status");
    }
    // Wifi status
    {
        bool wifi_connected = get_var_wifiConnected(); // true when connected
        bool local_wifi = get_var_localWifi(); // true when local wifi started

        if (!wifi_connected && !local_wifi) {
            lv_obj_add_flag(objects.wifi_local_logo, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.wifi_logo, LV_OBJ_FLAG_HIDDEN);
        } 
        else if (wifi_connected) {
            lv_obj_clear_flag(objects.wifi_logo, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.wifi_local_logo, LV_OBJ_FLAG_HIDDEN);
        }
        else if (local_wifi) {
            lv_obj_clear_flag(objects.wifi_local_logo, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.wifi_logo, LV_OBJ_FLAG_HIDDEN);
        }

        LV_LOG_INFO("Updated WiFi status");
    }
    // Custom Frequency Notification
    {
        bool value = get_var_customFreqEnabled();
        if (value && !laserAlert) {
            lv_label_set_text(objects.custom_freq_en, ".");
            if (lv_obj_has_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN)) { 
                lv_obj_clear_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN);
            }
        }
        else if (value && laserAlert) {
            lv_label_set_text(objects.custom_freq_en, "");
            lv_obj_add_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN);
        }

        LV_LOG_INFO("Updated Custom Frequency status");
    }
    // GPS status
    {
        bool gps_available = get_var_gpsAvailable(); // true if connected
    
        lv_obj_clear_flag(objects.nav_logo_enabled, gps_enabled ? LV_OBJ_FLAG_HIDDEN : 0);
        lv_obj_add_flag(objects.nav_logo_enabled, gps_enabled ? 0 : LV_OBJ_FLAG_HIDDEN);
    
        lv_obj_clear_flag(objects.nav_logo_disabled, gps_enabled ? 0 : LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.nav_logo_disabled, gps_enabled ? LV_OBJ_FLAG_HIDDEN : 0);
        LV_LOG_INFO("Updated GPS status");
    }
}

void tick_alertTable() {
    bool muted = get_var_muted();
    bool muteToGray = get_var_muteToGray();
    // Alert Table Freq & Direction update
    {
        int alertTableSize = get_var_alertTableSize();
        //bool needsUpdate = (alertTableSize != cur_alert_count);

        if (alertTableSize > 0) {
            LV_LOG_INFO("update alert table");
            const char** frequencies = get_var_frequencies();
            const char** directions = get_var_directions();
            update_alert_rows(alertTableSize, frequencies, muted, muteToGray);
            update_alert_arrows(alertTableSize, directions, muted, muteToGray);
            //cur_alert_count = alertTableSize;
        }
    }
    // Alert Table visibility
    {
        bool new_val = get_showAlertTable(); // true if alert table should display (more than 1 alert)
        tick_value_change_obj = objects.alert_table;
        if (new_val) {
            LV_LOG_INFO("update alert table");
            lv_obj_clear_flag(objects.alert_table, LV_OBJ_FLAG_HIDDEN);
        }
        else { lv_obj_add_flag(objects.alert_table, LV_OBJ_FLAG_HIDDEN); }
        tick_value_change_obj = NULL;
    }
}

void tick_screen_main() {
    bool alertPresent = get_var_alertPresent();
    int alertCount = get_var_alertCount();
    bool showBogeys = get_var_showBogeys();
    bool laserAlert = get_var_laserAlert();
    if (alertPresent) {
        uint32_t now = lv_tick_get();
        bool muted = get_var_muted();
        bool muteToGray = get_var_muteToGray();

        if (muteToGray) {
            update_alert_display(muted);
        }

        // Front Arrow
        {
            bool should_blink = blink_enabled[BLINK_FRONT];
            if (should_blink) {
                LV_LOG_INFO("paint front blink");
                if (now - last_blink_time >= BLINK_FREQUENCY) {
                    last_blink_time = now;
                    blink_state = !blink_state;

                    if (blink_state) {
                        lv_obj_clear_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN);
                    } else {
                        lv_obj_add_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN);
                    }
                }
            } else {
                bool new_val = get_var_arrowPrioFront(); // true when front should paint
                bool cur_val = lv_obj_has_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN); // true if "hidden"

                if (new_val == cur_val) {
                    LV_LOG_INFO("paint front solid");
                    tick_value_change_obj = objects.front_arrow;
                    if (new_val) {
                        lv_obj_clear_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN);
                    }
                    else lv_obj_add_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN);
                    tick_value_change_obj = NULL;
                }
            }
        }
        // Side Arrow
        { 
            //uint32_t now = lv_tick_get();
            bool should_blink = blink_enabled[BLINK_SIDE];
            if (should_blink) {
                LV_LOG_INFO("paint side blink");
                if (now - last_blink_time >= BLINK_FREQUENCY) {
                    last_blink_time = now;
                    blink_state = !blink_state;

                    if (blink_state) {
                        lv_obj_clear_flag(objects.side_arrow, LV_OBJ_FLAG_HIDDEN);
                    } else {
                        lv_obj_add_flag(objects.side_arrow, LV_OBJ_FLAG_HIDDEN);
                    }
                }
            } else {
                bool new_val = get_var_arrowPrioSide(); // true when side should paint
                bool cur_val = lv_obj_has_flag(objects.side_arrow, LV_OBJ_FLAG_HIDDEN); // true if "hidden"

                if (new_val == cur_val) {
                    LV_LOG_INFO("paint side solid");
                    tick_value_change_obj = objects.side_arrow;
                    if (new_val) lv_obj_clear_flag(objects.side_arrow, LV_OBJ_FLAG_HIDDEN);
                    else lv_obj_add_flag(objects.side_arrow, LV_OBJ_FLAG_HIDDEN);
                    tick_value_change_obj = NULL;
                }
            }
        }
        // Rear Arrow
        {
            //uint32_t now = lv_tick_get();
            bool should_blink = blink_enabled[BLINK_REAR];
            if (should_blink) {
                LV_LOG_INFO("paint rear blink");
                if (now - last_blink_time >= BLINK_FREQUENCY) {
                    last_blink_time = now;
                    blink_state = !blink_state;

                    if (blink_state) {
                        lv_obj_clear_flag(objects.rear_arrow, LV_OBJ_FLAG_HIDDEN);
                    } else {
                        lv_obj_add_flag(objects.rear_arrow, LV_OBJ_FLAG_HIDDEN);
                    }
                }
            } else {
                bool new_val = get_var_arrowPrioRear(); // true when rear should paint
                bool cur_val = lv_obj_has_flag(objects.rear_arrow, LV_OBJ_FLAG_HIDDEN); // true if "hidden"

                if (new_val == cur_val) {
                    LV_LOG_INFO("paint rear solid");
                    tick_value_change_obj = objects.rear_arrow;
                    if (new_val) lv_obj_clear_flag(objects.rear_arrow, LV_OBJ_FLAG_HIDDEN);
                    else lv_obj_add_flag(objects.rear_arrow, LV_OBJ_FLAG_HIDDEN);
                    tick_value_change_obj = NULL;
                }
            }
        }
        // Priority Alert Frequency & Bars
        {    
            const char *new_val = get_var_prio_alert_freq();
            const char *cur_val = lv_label_get_text(objects.prioalertfreq);

            if (strcmp(new_val, cur_val) != 0) {
                tick_value_change_obj = objects.prioalertfreq;
                if (new_val) { 
                    LV_LOG_INFO("updating prioAlert freq");
                    lv_label_set_text(objects.prioalertfreq, new_val);
                }
                tick_value_change_obj = NULL;
            } 
        }
        // Update Priority Bars
        {
            int numBars = get_var_prioBars();
            if (numBars != cur_bars) {
                tick_value_change_obj = objects.bar_str;
                LV_LOG_INFO("updating signal bars");
                update_signal_bars(numBars);
                cur_bars = numBars;
                tick_value_change_obj = NULL;
            }
        }  
        // Mute status
        {
            bool cur_val = lv_obj_has_flag(objects.mute_logo, LV_OBJ_FLAG_HIDDEN); // true if hidden
            if (muted == cur_val) {
                LV_LOG_INFO("mute updated");
                tick_value_change_obj = objects.mute_logo;
                if (muted) lv_obj_clear_flag(objects.mute_logo, LV_OBJ_FLAG_HIDDEN);
                else lv_obj_add_flag(objects.mute_logo, LV_OBJ_FLAG_HIDDEN);
                tick_value_change_obj = NULL;
            }
        }
        // Laser alert
        // {
        //     bool new_val = get_var_laserAlert(); // true if enabled
        //     bool is_hidden = lv_obj_has_flag(objects.band_laser, LV_OBJ_FLAG_HIDDEN); // true if hidden
        //     if (new_val == is_hidden) {
        //         LV_LOG_INFO("paint laser");
        //         tick_value_change_obj = objects.band_laser;
        //         if (new_val) { 
        //             lv_obj_clear_flag(objects.band_laser, LV_OBJ_FLAG_HIDDEN);
        //         }
        //         else {
        //             lv_obj_add_flag(objects.band_laser, LV_OBJ_FLAG_HIDDEN);
        //         }
        //         tick_value_change_obj = NULL;
        //     }
        // }

        // KA alert
        {
            bool new_val = get_var_kaAlert();  // true if enabled
            bool is_hidden = lv_obj_has_flag(objects.band_ka, LV_OBJ_FLAG_HIDDEN);

            if (new_val != !is_hidden) {
                LV_LOG_INFO("paint ka");
                tick_value_change_obj = objects.band_ka;
                new_val ? lv_obj_clear_flag(objects.band_ka, LV_OBJ_FLAG_HIDDEN) 
                        : lv_obj_add_flag(objects.band_ka, LV_OBJ_FLAG_HIDDEN);
                tick_value_change_obj = NULL;
            }
        }

        // K alert
        {
            bool new_val = get_var_kAlert(); // true if enabled
            bool is_hidden = lv_obj_has_flag(objects.band_k, LV_OBJ_FLAG_HIDDEN); // true if hidden

            if (new_val == is_hidden) {
                LV_LOG_INFO("paint k");
                tick_value_change_obj = objects.band_k;
                if (new_val) { 
                    lv_obj_clear_flag(objects.band_k, LV_OBJ_FLAG_HIDDEN);
                }
                else {
                    lv_obj_add_flag(objects.band_k, LV_OBJ_FLAG_HIDDEN);
                }
                tick_value_change_obj = NULL;
            }
        }
        // X alert
        {
            bool new_val = get_var_xAlert(); // true if enabled
            bool is_hidden = lv_obj_has_flag(objects.band_x, LV_OBJ_FLAG_HIDDEN); // true if hidden

            if (new_val == is_hidden) {
                LV_LOG_INFO("paint x");
                tick_value_change_obj = objects.band_x;
                if (new_val) { 
                    lv_obj_clear_flag(objects.band_x, LV_OBJ_FLAG_HIDDEN);
                }
                else { 
                    lv_obj_add_flag(objects.band_x, LV_OBJ_FLAG_HIDDEN);
                }
                tick_value_change_obj = NULL;
            }
        }
        // {
        //  TODO: anything related to vehicle speed?
        //     const char *new_val = get_var_lowspeedthreshold();
        //     const char *cur_val = lv_label_get_text(objects.vehiclespeed);
        //     if (strcmp(new_val, cur_val) != 0) {
        //         tick_value_change_obj = objects.vehiclespeed;
        //         lv_label_set_text(objects.vehiclespeed, new_val);
        //         tick_value_change_obj = NULL;
        //     }
        // }
    }
    // Logic Mode
    {
        bool useDefault = get_var_useDefaultV1Mode();
        const char *new_val = get_var_logicmode(useDefault);
        lv_obj_t *target = useDefault ? objects.default_mode : objects.mode_type;
        lv_obj_t *target_old = useDefault ? objects.mode_type : objects.default_mode;
        lv_obj_t *overlay = objects.overlay_mode;
        
        if (!target) {
            LV_LOG_ERROR("Error: %s is NULL!", useDefault ? "objects.default_mode" : "objects.mode_type");
            return;
        }
    
        if (!new_val) {
            LV_LOG_ERROR("Error: get_var_logicmode() returned NULL!");
            return;
        }
    
        const char *cur_val = lv_label_get_text(target);
        if (cur_val && strcmp(new_val, cur_val) != 0) {
            LV_LOG_INFO("update Logic Mode");
            tick_value_change_obj = target;
            lv_label_set_text(target, new_val);
            lv_label_set_text(target_old, "");
    
            lv_obj_clear_flag(target, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_set_style_text_color(target, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(target_old, LV_OBJ_FLAG_HIDDEN);

            if (new_val == "c") {
                lv_label_set_text(overlay, "q");
                lv_obj_clear_flag(overlay, LV_OBJ_FLAG_HIDDEN);
            }
        }

        if (alertPresent && showBogeys && alertCount > 0 && !laserAlert) {
            lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_fmt(objects.default_mode, "%d", alertCount);
        }
        else if (alertPresent || laserAlert) {
            lv_obj_add_flag(objects.default_mode, LV_OBJ_FLAG_HIDDEN);
        } 
        else {
            //lv_obj_clear_flag(objects.default_mode, LV_OBJ_FLAG_HIDDEN);
        }
        tick_value_change_obj = NULL;
    }
}

void create_screen_settings() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 600, 450);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    {
        lv_obj_t *parent_obj = obj;
        {
            // label_ip
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_ip = obj;
            //lv_obj_set_pos(obj, 98, 258);
            lv_obj_set_pos(obj, 98, 157);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "IP:");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_ip_val
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_ip_val = obj;
            //lv_obj_set_pos(obj, 262, 258);
            lv_obj_set_pos(obj, 262, 157);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "0.0.0.0");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_ssid
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_ssid = obj;
            //lv_obj_set_pos(obj, 98, 157);
            lv_obj_set_pos(obj, 98, 208);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "SSID:");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_ssid_val
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_ssid_val = obj;
            //lv_obj_set_pos(obj, 262, 157);
            lv_obj_set_pos(obj, 262, 208);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL);
            lv_label_set_text(obj, "default");
            lv_obj_set_scroll_dir(obj, LV_DIR_LEFT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_pass
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_pass = obj;
            //lv_obj_set_pos(obj, 98, 208);
            lv_obj_set_pos(obj, 98, 258);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "PASS:");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_pass_val
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_pass_val = obj;
            //lv_obj_set_pos(obj, 262, 208);
            lv_obj_set_pos(obj, 262, 258);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL);
            lv_label_set_text(obj, "default");
            lv_obj_set_scroll_dir(obj, LV_DIR_LEFT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // slider_brightness
            lv_obj_t *obj = lv_slider_create(parent_obj);
            objects.slider_brightness = obj;
            lv_obj_set_pos(obj, 168, 45);
            lv_obj_set_size(obj, 402, 48);
            lv_slider_set_range(obj, 10, 255);
            lv_slider_set_value(obj, 200, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(obj, lv_color_hex(default_color), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(objects.slider_brightness, slider_brightness_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            // label_brightness
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_brightness = obj;
            lv_obj_set_pos(obj, 264, 6);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "BRIGHTNESS");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_wifi
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_wifi = obj;
            lv_obj_set_pos(obj, 14, 6);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "WIFI");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // switch_wifi
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.switch_wifi = obj;
            lv_obj_set_pos(obj, 14, 45);
            lv_obj_set_size(obj, 76, 48);
            lv_obj_set_style_bg_color(obj, lv_color_hex(default_color), LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_add_event_cb(objects.switch_wifi, wifi_switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            // label_v1cle
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_v1cle = obj;
            lv_obj_set_pos(obj, 14, 360);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "V1CLE");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff517bd6), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            //lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // switch_v1cle
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.switch_v1cle = obj;
            lv_obj_set_pos(obj, 14, 400);
            lv_obj_set_size(obj, 76, 48);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff517bd6), LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_add_event_cb(objects.switch_v1cle, v1cle_switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
            //lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // label_ble_rssi
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_ble_rssi = obj;
            lv_obj_set_pos(obj, 362, 418);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "BLE");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff517bd6), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_ble_rssi_val
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_ble_rssi_val = obj;
            lv_obj_set_pos(obj, 437, 418);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "0 dBm");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff517bd6), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_wifi_rssi_val
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_wifi_rssi_val = obj;
            lv_obj_set_pos(obj, 437, 386);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "0 dBm");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // label_wifi_rssi
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_wifi_rssi = obj;
            lv_obj_set_pos(obj, 343, 386);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "WIFI");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_settings();
}

void tick_screen_settings() {
    // WiFi SSID
    {
        const char *new_val = get_var_ssid();
        const char *cur_val = (objects.label_ssid_val) ? lv_label_get_text(objects.label_ssid_val) : NULL;
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.label_ssid_val;
            LV_LOG_INFO("updating SSID");
            lv_label_set_text(objects.label_ssid_val, new_val);
            tick_value_change_obj = NULL;
        }
    }
    // WiFi password
    {
        //const char *cur_val = (objects.label_pass_val) ? lv_label_get_text(objects.label_pass_val) : NULL;
        bool show = get_var_wifiConnected();
        if (show) {
            lv_obj_add_flag(objects.label_pass_val, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.label_pass, LV_OBJ_FLAG_HIDDEN);
        } else {
            const char *password = get_var_password();
            {
                tick_value_change_obj = objects.label_pass_val;
                LV_LOG_INFO("updating password");
                lv_label_set_text(objects.label_pass_val, password);
                lv_obj_clear_flag(objects.label_pass_val, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.label_pass, LV_OBJ_FLAG_HIDDEN);
                tick_value_change_obj = NULL;
            }
        }
    }
    // IP address
    {
        const char *new_val = get_var_ipAddress();
        const char *cur_val = (objects.label_ip_val) ? lv_label_get_text(objects.label_ip_val) : NULL;
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.label_ip_val;
            LV_LOG_INFO("updating IP address");
            lv_label_set_text(objects.label_ip_val, new_val);
            tick_value_change_obj = NULL;
        }
    }
    // WiFi enable/disable
    {
        if (!objects.switch_wifi) return;

        bool wifi_enabled = get_var_wifiEnabled();
        bool switch_checked = lv_obj_has_state(objects.switch_wifi, LV_STATE_CHECKED);
    
        if (wifi_enabled && !switch_checked) {
            lv_obj_add_state(objects.switch_wifi, LV_STATE_CHECKED);
        } else if (!wifi_enabled && switch_checked) {
            lv_obj_clear_state(objects.switch_wifi, LV_STATE_CHECKED);
        }
    }
    // V1C LE enable/disable
    {
        if (!objects.switch_v1cle) return;

        bool v1cle_present = get_var_v1clePresent();
        bool usev1cle = get_var_usev1cle();
        bool switch_checked = lv_obj_has_state(objects.switch_v1cle, LV_STATE_CHECKED);

        if (!v1cle_present) {
            // TODO: fix BLE scan timer to capture this in case v1g is found first
            //lv_obj_add_flag(objects.label_v1cle, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_add_flag(objects.switch_v1cle, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            if (usev1cle && !switch_checked) {
                lv_obj_clear_flag(objects.label_v1cle, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.switch_v1cle, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_state(objects.switch_v1cle, LV_STATE_CHECKED);
            }
            else if (usev1cle && switch_checked) {
                lv_obj_clear_flag(objects.label_v1cle, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.switch_v1cle, LV_OBJ_FLAG_HIDDEN);
                //lv_obj_clear_state(objects.switch_v1cle, LV_STATE_CHECKED);
            }
        }
    }
    // RSSI vals
    {
        static unsigned long lastUpdateTime = 0;
        unsigned long currentTime = getMillis();

        if (currentTime - lastUpdateTime < 2000) {
            return;
        }

        lastUpdateTime = currentTime;

        int bleRssi = getBluetoothSignalStrength();
        int wifiRssi = getWifiRSSI();
        char bleRssi_str[8];
        char wifiRssi_str[8];
        snprintf(bleRssi_str, sizeof(bleRssi_str), "%d dBm", bleRssi);
        snprintf(wifiRssi_str, sizeof(wifiRssi_str), "%d dBm", wifiRssi);
        
        tick_value_change_obj = objects.label_ble_rssi_val;
        LV_LOG_INFO("updating BLE RSSI");
        lv_label_set_text(objects.label_ble_rssi_val, bleRssi_str);
        LV_LOG_INFO("updating Wifi RSSI");
        tick_value_change_obj = objects.label_wifi_rssi_val;
        lv_label_set_text(objects.label_wifi_rssi_val, wifiRssi_str);
        tick_value_change_obj = NULL;
    }
}

void create_screen_dispSettings() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.dispSettings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 600, 450);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(objects.dispSettings, gesture_event_handler, LV_EVENT_GESTURE, NULL);

    {
        lv_obj_t *parent_obj = obj;
        {
            // mutetogray_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.mutetogray_label = obj;
            lv_obj_set_pos(obj, 20, 16);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "MUTE-TO-GRAY");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // mutetogray_button
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.mutetogray_button = obj;
            lv_obj_set_pos(obj, 500, 10);
            lv_obj_set_size(obj, 76, 36);
            lv_obj_set_style_bg_color(obj, lv_color_hex(default_color), LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_add_event_cb(objects.mutetogray_button, muteToGray_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            // colorbars_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.colorbars_label = obj;
            lv_obj_set_pos(obj, 20, 66);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "COLOR BARS");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // colorbars_button
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.colorbars_button = obj;
            lv_obj_set_pos(obj, 500, 60);
            lv_obj_set_size(obj, 76, 36);
            lv_obj_set_style_bg_color(obj, lv_color_hex(default_color), LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_add_event_cb(objects.colorbars_button, colorBars_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            // defaultmode_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.defaultmode_label = obj;
            lv_obj_set_pos(obj, 20, 116);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "USE DEFAULT MODE");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // defaultmode_button
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.defaultmode_button = obj;
            lv_obj_set_pos(obj, 500, 110);
            lv_obj_set_size(obj, 76, 36);
            lv_obj_set_style_bg_color(obj, lv_color_hex(default_color), LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_add_event_cb(objects.defaultmode_button, defaultMode_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            // showbogeys_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.showbogeys_label = obj;
            lv_obj_set_pos(obj, 20, 166);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "SHOW BOGEY COUNTER");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // showbogeys_button
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.showbogeys_button = obj;
            lv_obj_set_pos(obj, 500, 160);
            lv_obj_set_size(obj, 76, 36);
            lv_obj_set_style_bg_color(obj, lv_color_hex(default_color), LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_add_event_cb(objects.showbogeys_button, bogeyCounter_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            // quietride_dropdown
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.quietride_dropdown = obj;
            lv_obj_set_pos(obj, 426, 387);
            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "20\n25\n30\n35\n40\n45\n50\n55\n60");
            lv_dropdown_set_dir(obj, LV_DIR_TOP);
            lv_obj_add_event_cb(obj, speedThreshold_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            // quietride_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.quietride_label = obj;
            lv_obj_set_pos(obj, 20, 391);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "QUIET RIDE SPEED");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // unit of speed label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.unit_of_speed_label = obj;
            lv_obj_set_pos(obj, 20, 327);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "UNIT OF SPEED");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // unit of speed dropdown
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.unit_of_speed_dropdown = obj;
            lv_obj_set_pos(obj, 426, 323);
            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "MPH\nKPH");
            lv_dropdown_set_dir(obj, LV_DIR_TOP);
            lv_obj_add_event_cb(obj, unitOfSpeed_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            // blankscreen_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.blankscreen_label = obj;
            lv_obj_set_pos(obj, 20, 216);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "BLANK V1");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // blankscreen_button
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.blankscreen_button = obj;
            lv_obj_set_pos(obj, 500, 210);
            lv_obj_set_size(obj, 76, 36);
            lv_obj_set_style_bg_color(obj, lv_color_hex(default_color), LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_add_event_cb(objects.blankscreen_button, blankV1display_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
        {
            // show_bt_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.show_bt_label = obj;
            lv_obj_set_pos(obj, 20, 266);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "SHOW BT ICON");
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // show_bt_button
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.show_bt_button = obj;
            lv_obj_set_pos(obj, 500, 263);
            lv_obj_set_size(obj, 76, 36);
            lv_obj_set_style_bg_color(obj, lv_color_hex(default_color), LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_event_cb(objects.show_bt_button, showBT_handler, LV_EVENT_VALUE_CHANGED, NULL);
        }
    }
    
    tick_screen_dispSettings();
}

void tick_screen_dispSettings() {
    // Populate unit of speed drop-down
    {
        bool useImperial = get_var_useImperial();
        
        if (useImperial) {
            lv_dropdown_set_selected(objects.unit_of_speed_dropdown, 0);
        } else {
            lv_dropdown_set_selected(objects.unit_of_speed_dropdown, 1);
        }
    }
    // Populate SilentRide threshold
    {
        static int last_threshold = -1;
        int threshold = get_var_speedThreshold();

        if (threshold == last_threshold) {
            return;
        }

        switch (threshold) {
            case 20:
                lv_dropdown_set_selected(objects.quietride_dropdown, 0);
                break;
            case 25:
                lv_dropdown_set_selected(objects.quietride_dropdown, 1);
                break;
            case 30:
                lv_dropdown_set_selected(objects.quietride_dropdown, 2);
                break;
            case 35:
                lv_dropdown_set_selected(objects.quietride_dropdown, 3);
                break;
            case 40:
                lv_dropdown_set_selected(objects.quietride_dropdown, 4);
                break;
            case 45:
                lv_dropdown_set_selected(objects.quietride_dropdown, 5);
                break;
            case 50:
                lv_dropdown_set_selected(objects.quietride_dropdown, 6);
                break;
            case 55:
                lv_dropdown_set_selected(objects.quietride_dropdown, 7);
                break;
            case 60:
                lv_dropdown_set_selected(objects.quietride_dropdown, 8);
                break;
            default:
                lv_dropdown_set_selected(objects.quietride_dropdown, 0);
                break;
        }
    }
    // Show or hide the BT icon setting
    {
        bool showBTSetting = get_var_blankDisplay();
        if (showBTSetting) {
            lv_obj_clear_flag(objects.show_bt_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.show_bt_button, LV_OBJ_FLAG_HIDDEN);
            
            bool showBT = get_var_dispBTIcon();
            if (showBT) {
                bool switch_checked = lv_obj_has_state(objects.show_bt_button, LV_STATE_CHECKED);
            
                if (showBT && !switch_checked) {
                    lv_obj_add_state(objects.show_bt_button, LV_STATE_CHECKED);
                } else if (!showBT && switch_checked) {
                    lv_obj_clear_state(objects.show_bt_button, LV_STATE_CHECKED);
                }
            } else {
                // anything to be done here?
            }
        }
    }
    // Blank V1 Display
    {
        bool showBlankDisp = get_var_blankDisplay();
        if (showBlankDisp) {
            bool switch_checked = lv_obj_has_state(objects.blankscreen_button, LV_STATE_CHECKED);

            if (showBlankDisp && !switch_checked) {
                lv_obj_add_state(objects.blankscreen_button, LV_STATE_CHECKED);
            } else if (!showBlankDisp && switch_checked) {
                lv_obj_clear_state(objects.blankscreen_button, LV_STATE_CHECKED);
            }
        }
    }
    // Mute to Gray
    {
        bool muteToGray = get_var_muteToGray();
        if (muteToGray) {
            bool switch_checked = lv_obj_has_state(objects.mutetogray_button, LV_STATE_CHECKED);

            if (muteToGray && !switch_checked) {
                lv_obj_add_state(objects.mutetogray_button, LV_STATE_CHECKED);
            } else if (!muteToGray && switch_checked) {
                lv_obj_clear_state(objects.mutetogray_button, LV_STATE_CHECKED);
            }
        }
    }
    // Bogey Counter
    {
        bool showBogeyCounter = get_var_showBogeys();
        if (showBogeyCounter) {
            bool switch_checked = lv_obj_has_state(objects.showbogeys_button, LV_STATE_CHECKED);

            if (showBogeyCounter && !switch_checked) {
                lv_obj_add_state(objects.showbogeys_button, LV_STATE_CHECKED);
            } else if (!showBogeyCounter && switch_checked) {
                lv_obj_clear_state(objects.showbogeys_button, LV_STATE_CHECKED);
            }
        }
    }
    // Color Bars
    {
        bool colorBars = get_var_colorBars();
        if (colorBars) {
            bool switch_checked = lv_obj_has_state(objects.colorbars_button, LV_STATE_CHECKED);

            if (colorBars && !switch_checked) {
                lv_obj_add_state(objects.colorbars_button, LV_STATE_CHECKED);
            } else if (!colorBars && switch_checked) {
                lv_obj_clear_state(objects.colorbars_button, LV_STATE_CHECKED);
            }
        }
    }
    // Default Mode
    {
        bool defaultMode = get_var_useDefaultV1Mode();
        if (defaultMode) {
            bool switch_checked = lv_obj_has_state(objects.defaultmode_button, LV_STATE_CHECKED);

            if (defaultMode && !switch_checked) {
                lv_obj_add_state(objects.defaultmode_button, LV_STATE_CHECKED);
            } else if (!defaultMode && switch_checked) {
                lv_obj_clear_state(objects.defaultmode_button, LV_STATE_CHECKED);
            }
        }
    }
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_logo_screen();
    create_screen_main();
    create_screen_settings();
    create_screen_dispSettings();

    //lv_obj_add_event_cb(objects.main, gesture_event_handler, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(objects.settings, gesture_event_handler, LV_EVENT_GESTURE, NULL);

    lv_scr_load(objects.logo_screen);
    currentScreen = SCREEN_ID_LOGO_SCREEN;
}

typedef void (*tick_screen_func_t)();

tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_logo_screen,
    tick_screen_main,
    tick_screen_settings,
    tick_screen_dispSettings,
};

void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
