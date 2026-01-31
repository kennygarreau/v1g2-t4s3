#include <string.h>
#include <stdio.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "../utils.h"
#include "blinking.h"

objects_t objects;
lv_obj_t *tick_value_change_obj;
lv_obj_t* signal_bars[MAX_BARS];
lv_obj_t* alert_rows[MAX_ALERT_ROWS];
lv_obj_t* alert_directions[MAX_ALERT_ROWS];

static uint32_t last_blink_time = 0;
static bool blink_state = false;

uint32_t default_color = 0xffff0000;
uint32_t gray_color = 0xff636363;
uint32_t yellow_color = 0xffda954b;

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
    int base_x = 100;       // Base X position for all bars
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

    LV_LOG_INFO("update_signal_bars to %d", num_visible);
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
        // prioalertfreq
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.prioalertfreq = obj;
            lv_obj_set_pos(obj, 120, 341);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_112, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        // CREATE FLEX CONTAINER FOR STATUS ICONS
        {
            lv_obj_t *container = lv_obj_create(parent_obj);
            objects.status_icon_container = container;
            
            // Position container in top-right corner
            lv_obj_align(container, LV_ALIGN_TOP_RIGHT, 0, 0);
            lv_obj_set_size(container, LV_SIZE_CONTENT, 40);  // Auto width, fixed height
            
            // Configure flex layout (right-to-left row)
            lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW_REVERSE);
            lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            
            // Set spacing between icons
            lv_obj_set_style_pad_column(container, -28, 0);
            lv_obj_set_style_pad_all(container, 0, 0);
            lv_obj_set_style_pad_row(container, 0, 0);
            
            // Make container transparent
            lv_obj_set_style_bg_opa(container, 0, 0);
            lv_obj_set_style_border_width(container, 0, 0);
            
            // Disable scrolling
            lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_clip_corner(container, false, 0);
        }
        
        // NOW CREATE ICONS AS CHILDREN OF THE CONTAINER
        // Order matters: rightmost icon should be created first
        
        // bt_logo (rightmost)
        {
            lv_obj_t *obj = lv_img_create(objects.status_icon_container);
            objects.bt_logo = obj;
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_bt_logo_small);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        
        // bt_proxy_logo (alternative to bt_logo, same position)
        {
            lv_obj_t *obj = lv_img_create(objects.status_icon_container);
            objects.bt_proxy_logo = obj;
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_bt_proxy);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        
        // wifi_logo
        {
            lv_obj_t *obj = lv_img_create(objects.status_icon_container);
            objects.wifi_logo = obj;
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_wifi);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        
        // wifi_local_logo
        {
            lv_obj_t *obj = lv_img_create(objects.status_icon_container);
            objects.wifi_local_logo = obj;
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_wifi_local);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        
        // wifi_localConnected
        {
            lv_obj_t *obj = lv_img_create(objects.status_icon_container);
            objects.wifi_localConnected = obj;
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_wifi_localConnected);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        
        // nav_logo_enabled
        {
            lv_obj_t *obj = lv_img_create(objects.status_icon_container);
            objects.nav_logo_enabled = obj;
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_location_red);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        
        // nav_logo_disabled
        {
            lv_obj_t *obj = lv_img_create(objects.status_icon_container);
            objects.nav_logo_disabled = obj;
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_location_disabled);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        
        // mute_logo (leftmost)
        {
            lv_obj_t *obj = lv_img_create(objects.status_icon_container);
            objects.mute_logo = obj;
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_mute_logo_small);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        // photo_radar_type
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.photo_type = obj;
            lv_obj_set_pos(obj, 148, 16);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(yellow_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        // photo_radar_icon
        {
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.photo_image = obj;
            lv_obj_set_pos(obj, 7, 332);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&ui_image_photo_camera);
            lv_img_set_src(obj, psram_img);
            lv_img_set_zoom(obj, 256);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        // mode_type
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.mode_type = obj;
            lv_obj_set_pos(obj, 17, 16);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        // bogey count
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.bogey_count = obj;
            lv_obj_set_pos(obj, 17, 50);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_128, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        // default v1 mode
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.default_mode = obj;
            lv_obj_set_pos(obj, 17, 341);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_128, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

            // Create the black "3" overlay
            lv_obj_t *overlay = lv_label_create(parent_obj);
            objects.overlay_mode = overlay;
            lv_obj_set_pos(overlay, 17, 341); // Same position as default_mode
            lv_obj_set_size(overlay, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(overlay, "");
            lv_obj_set_style_text_font(overlay, &ui_font_alarmclock_128, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(overlay, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
        }
        // custom freqs enabled
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.custom_freq_en = obj;
            lv_obj_set_pos(obj, 60, 341);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_128, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(default_color), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        // alert_table
        {
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
        // main_signal_bars
        {
            create_signal_bars(parent_obj, MAX_BARS);
        }
        // front arrow
        { 
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.front_arrow = obj;
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_arrow_front_4bit);
            lv_img_set_src(obj, psram_img);
            lv_obj_align(obj, LV_ALIGN_CENTER, -24, -70);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        // side arrow
        {
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.side_arrow = obj;
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_arrow_side_4bit);
            lv_img_set_src(obj, psram_img);
            lv_obj_align(obj, LV_ALIGN_CENTER, -24, 0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        // rear arrow
        {
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.rear_arrow = obj;
            lv_img_dsc_t *psram_img = allocate_image_in_psram(&img_arrow_rear_4bit);
            lv_img_set_src(obj, psram_img);
            lv_obj_align(obj, LV_ALIGN_CENTER, -24, 50);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // band_ka
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.band_ka = obj;
            lv_obj_set_pos(obj, 14, 162);
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
            lv_obj_set_pos(obj, 14, 200);
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
            lv_obj_set_pos(obj, 14, 238);
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
        static bool last_bt_connected = false;
        static bool last_proxy_connected = false;

        bool bt_connected = get_var_bt_connected(); // true when connected
        bool proxy_connected = get_var_proxyConnected();

        if (bt_connected != last_bt_connected || proxy_connected != last_proxy_connected) {
            if (!bt_connected) {
                lv_obj_add_flag(objects.bt_logo, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.bt_proxy_logo, LV_OBJ_FLAG_HIDDEN);
            } 
            else {
                if (proxy_connected) {
                    lv_obj_clear_flag(objects.bt_proxy_logo, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(objects.bt_logo, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_clear_flag(objects.bt_logo, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(objects.bt_proxy_logo, LV_OBJ_FLAG_HIDDEN);
                }
            }

            LV_LOG_INFO("Updated Bluetooth/proxy status");
            last_bt_connected = bt_connected;
            last_proxy_connected = proxy_connected;
        }
    }
    // Wifi status
    {
        static bool last_wifi_connected = false;
        static bool last_local_wifi = false;
        static bool last_wifiLocalConnected = false;
        
        bool wifiLocalConnected = get_var_wifiClientConnected();
        bool wifi_connected = get_var_wifiConnected(); // true when connected
        bool local_wifi = get_var_localWifi(); // true when local wifi started
        bool wifi_enabled = get_var_wifiEnabled();

        if (wifi_connected != last_wifi_connected || 
            local_wifi != last_local_wifi || 
            wifiLocalConnected != last_wifiLocalConnected) {
            
            lv_obj_add_flag(objects.wifi_logo, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.wifi_local_logo, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.wifi_localConnected, LV_OBJ_FLAG_HIDDEN);
            
            if (wifi_connected) {
                // Connected to external WiFi - highest priority
                lv_obj_clear_flag(objects.wifi_logo, LV_OBJ_FLAG_HIDDEN);
            }
            else if (wifiLocalConnected) {
                // Local WiFi AP with client connected
                lv_obj_clear_flag(objects.wifi_localConnected, LV_OBJ_FLAG_HIDDEN);
            }
            else if (local_wifi) {
                // Local WiFi AP running but no clients
                lv_obj_clear_flag(objects.wifi_local_logo, LV_OBJ_FLAG_HIDDEN);
            }
            // else: all WiFi icons remain hidden (WiFi disabled or no connections)
            
            LV_LOG_INFO("Updated WiFi status");
            last_wifi_connected = wifi_connected;
            last_local_wifi = local_wifi;
            last_wifiLocalConnected = wifiLocalConnected;
        }
    }
    // Custom Frequency Notification
    {
        static bool lastValue = false;
        bool value = get_var_customFreqEnabled();

        if (value != lastValue) {
            if (value && !laserAlert) {
                lv_label_set_text(objects.custom_freq_en, ".");
                if (lv_obj_has_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN)) { 
                    lv_obj_clear_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN);
                }
            }
            else if (value && laserAlert) {
                //lv_label_set_text(objects.custom_freq_en, "");
                lv_obj_add_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN);
            } 
            else {
                lv_obj_add_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN);
            }

            LV_LOG_INFO("Updated Custom Frequency status");
            lastValue = value;
        }
    }
    // GPS status
    {
        static bool lastValue = false;
        bool gps_available = get_var_gpsAvailable(); // true if connected
    
        if (lastValue != gps_available) {
            lv_obj_clear_flag(objects.nav_logo_enabled, gps_enabled ? LV_OBJ_FLAG_HIDDEN : 0);
            lv_obj_add_flag(objects.nav_logo_enabled, gps_enabled ? 0 : LV_OBJ_FLAG_HIDDEN);
        
            lv_obj_clear_flag(objects.nav_logo_disabled, gps_enabled ? 0 : LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.nav_logo_disabled, gps_enabled ? LV_OBJ_FLAG_HIDDEN : 0);
            LV_LOG_INFO("Updated GPS status");
            lastValue = gps_available;
        }
    }
}

void tick_alertTable() {
    static int lastTableSize = -1;
    //static bool lastVisibility = false;
    static bool lastMuteState = false;

    // gather current state
    int currentSize = get_var_alertTableSize();
    bool currentVisibility = get_showAlertTable();
    bool currentMute = get_var_muted();
    bool currentMuteGray = get_var_muteToGray();

    // set visibility
    // if (currentVisibility != lastVisibility) {
    //     lastVisibility = currentVisibility;
        if (currentVisibility) {
            lv_obj_clear_flag(objects.alert_table, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(objects.alert_table, LV_OBJ_FLAG_HIDDEN);
        }
    //}

    // update alert table content based on size change OR mute status change
    if (currentVisibility && (currentSize != lastTableSize || currentMute != lastMuteState)) {
        lastTableSize = currentSize;
        lastMuteState = currentMute;

        if (currentSize > 0) {
            LV_LOG_INFO("update alert table content");
            const char** frequencies = get_var_frequencies();
            const char** directions = get_var_directions();

            update_alert_rows(currentSize, frequencies, currentMute, currentMuteGray);
            update_alert_arrows(currentSize, directions, currentMute, currentMuteGray);
        }
    }
}

void tick_screen_main() {
    bool alertPresent = get_var_alertPresent();
    uint8_t alertCount = get_var_alertCount();
    bool showBogeys = get_var_showBogeys();
    bool laserAlert = get_var_laserAlert();
    static bool barsCleared = true;
    static bool idleStateSet = false;

    if (alertPresent) {
        uint32_t now = lv_tick_get();
        static bool lastColorState = false;
        
        bool muted = get_var_muted();
        bool muteToGray = get_var_muteToGray();
        bool displayGray = (muted && muteToGray);

        if (displayGray != lastColorState) {
            update_alert_display(displayGray);
            lastColorState = displayGray;
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
                        //lv_obj_set_style_opa(objects.front_arrow, LV_OPA_COVER, 0);
                    } else {
                        lv_obj_add_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN);
                        //lv_obj_set_style_opa(objects.front_arrow, LV_OPA_TRANSP, 0);
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
                        //lv_obj_set_style_opa(objects.front_arrow, LV_OPA_COVER, 0);
                    }
                    lv_obj_add_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN);
                    //lv_obj_set_style_opa(objects.front_arrow, LV_OPA_TRANSP, 0);
                    tick_value_change_obj = NULL;
                }
            }
        }
        // Side Arrow
        { 
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
            static int lastBarLevel = -1;
            static uint32_t lastBarUpdateTime = 0;
            int numBars = get_var_prioBars();

            if ((numBars != lastBarLevel || (barsCleared && numBars > 0)) && 
                (getMillis() - lastBarUpdateTime > 200)) { {
                    lastBarLevel = numBars;
                    lastBarUpdateTime = getMillis();
                    barsCleared = false;

                    LV_LOG_INFO("updating signal bar count to %d", numBars);
                    update_signal_bars(numBars);
                }
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
        // KA alert
        {
            bool new_val = get_var_kaAlert();  // true if enabled
            bool is_hidden = lv_obj_has_flag(objects.band_ka, LV_OBJ_FLAG_HIDDEN);

            if (new_val == is_hidden) {
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
        // Photo Alert
        {
            bool new_val = get_var_photoAlertPresent(); // true if enabled
            bool is_hidden = lv_obj_has_flag(objects.photo_type, LV_OBJ_FLAG_HIDDEN);
            const char* photoType = get_var_photoType();

            if (new_val == is_hidden) {
                LV_LOG_INFO("paint photo radar");
                tick_value_change_obj = objects.photo_type;
                if (new_val) {
                    lv_obj_clear_flag(objects.photo_image, LV_OBJ_FLAG_HIDDEN);
                    lv_label_set_text(objects.photo_type, photoType);
                    lv_obj_clear_flag(objects.photo_type, LV_OBJ_FLAG_HIDDEN);
                }
                else {
                    lv_obj_add_flag(objects.photo_type, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(objects.photo_image, LV_OBJ_FLAG_HIDDEN);
                }
                tick_value_change_obj = NULL;
            }
        }
        
        idleStateSet = false;
    }
    else if (!idleStateSet) {
        LV_LOG_INFO("No alerts present; set idleState");
        update_signal_bars(0);
        barsCleared = true;
        set_var_showAlertTable(false);

        lv_label_set_text(objects.prioalertfreq, "");

        lv_obj_add_flag(objects.alert_table, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.mute_logo, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.photo_type, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.photo_image, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.band_x, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.band_k, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.band_ka, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.rear_arrow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.side_arrow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN);

        idleStateSet = true;
    }
    // Logic Mode OR Bogey Count
    {
        bool useDefault = get_var_useDefaultV1Mode();

        lv_obj_t *target = useDefault ? objects.default_mode : objects.mode_type;
        lv_obj_t *target_old = useDefault ? objects.mode_type : objects.default_mode;
        lv_obj_t *overlay = objects.overlay_mode;

        const char *new_val = get_var_logicmode(useDefault); // get the actual name of the logic mode, eg., "ADV LOGIC" or "L"
        const char *cur_val = lv_label_get_text(target);     // get the current value of the logic mode, eg., "ADV LOGIC", or "L"
        
        if (!target) {
            LV_LOG_ERROR("Error: %s is NULL!", useDefault ? "objects.default_mode" : "objects.mode_type");
            return;
        }
    
        if (!new_val) {
            LV_LOG_ERROR("Error: get_var_logicmode() returned NULL!");
            return;
        }
    
        static int lastAlertCount = -1;
        // Alert present = show the bogey counter, hide the mode + custom freq indicator
        if (alertPresent && useDefault && (lastAlertCount != alertCount)) {
            if (laserAlert) {
                LV_LOG_INFO("update UI to LASER alert");
                lv_obj_add_flag(objects.default_mode, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(objects.bogey_count, LV_OBJ_FLAG_HIDDEN);
            } else {
                LV_LOG_INFO("update UI bogey count to %d", alertCount);
                if (lv_obj_has_flag(objects.bogey_count, LV_OBJ_FLAG_HIDDEN)) {
                    lv_obj_clear_flag(objects.bogey_count, LV_OBJ_FLAG_HIDDEN);
                }
                if (!lv_obj_has_flag(objects.default_mode, LV_OBJ_FLAG_HIDDEN)) {
                    lv_obj_add_flag(objects.default_mode, LV_OBJ_FLAG_HIDDEN);
                }
                lv_label_set_text_fmt(objects.bogey_count, "%d", alertCount);
            }
            lastAlertCount = alertCount;
        }
        // No alert present = show the mode + custom freq indicator, hide the bogey counter
        else if (!alertPresent && useDefault) {
            if (new_val && cur_val && strcmp(new_val, cur_val) != 0 || 
                lv_obj_has_flag(objects.default_mode, LV_OBJ_FLAG_HIDDEN)) {

                    LV_LOG_INFO("updating mode to %s", new_val);
                    lv_label_set_text(target, new_val);
                    lv_label_set_text(target_old, "");
                    lv_label_set_text(objects.prioalertfreq, "");
                    // update_signal_bars(0);
                    // barsCleared = true;

                    if (lv_obj_has_flag(objects.default_mode, LV_OBJ_FLAG_HIDDEN)) {
                        lv_obj_clear_flag(objects.default_mode, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (lv_obj_has_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN)) {
                        lv_obj_clear_flag(objects.custom_freq_en, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (lv_obj_has_flag(target, LV_OBJ_FLAG_HIDDEN)) {
                        lv_obj_clear_flag(target, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (!lv_obj_has_flag(target_old, LV_OBJ_FLAG_HIDDEN)) {
                        lv_obj_add_flag(target_old, LV_OBJ_FLAG_HIDDEN);
                    }
                    if (!lv_obj_has_flag(objects.bogey_count, LV_OBJ_FLAG_HIDDEN)) {
                        lv_obj_add_flag(objects.bogey_count, LV_OBJ_FLAG_HIDDEN);
                    }
                    // set the overlay for advanced logic mode "small L"
                    if (new_val == "c") {
                        lv_label_set_text(overlay, "4");
                        lv_obj_clear_flag(overlay, LV_OBJ_FLAG_HIDDEN);
                    } else {
                        lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
                    }
                }

                lastAlertCount = -1;
        }
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
            lv_obj_set_style_text_font(obj, &ui_font_noto_speed_36, LV_PART_MAIN | LV_STATE_DEFAULT);
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
            lv_obj_set_style_text_font(obj, &ui_font_noto_speed_36, LV_PART_MAIN | LV_STATE_DEFAULT);
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
            lv_obj_set_style_text_font(obj, &ui_font_noto_speed_36, LV_PART_MAIN | LV_STATE_DEFAULT);
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
            // label_proxy
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_proxy = obj;
            lv_obj_set_pos(obj, 180, 360);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "PROXY");
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff5bb450), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            //lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // switch_proxy
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.switch_proxy = obj;
            lv_obj_set_pos(obj, 180, 400);
            lv_obj_set_size(obj, 76, 48);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff5bb450), LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_obj_add_event_cb(objects.switch_proxy, proxy_switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
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
    bool wifi_enabled = get_var_wifiEnabled();
    bool local_wifi_enabled = get_var_localWifi();
    // WiFi SSID
    {
        const char *new_val = get_var_ssid();
        const char *cur_val = (objects.label_ssid_val) ? lv_label_get_text(objects.label_ssid_val) : NULL;
        if (new_val == NULL) new_val = "";
        
        if (wifi_enabled) {
            if (strcmp(new_val, cur_val) != 0) {
                tick_value_change_obj = objects.label_ssid_val;
                LV_LOG_INFO("updating SSID");
                lv_obj_clear_flag(objects.label_ssid_val, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.label_ssid, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(objects.label_ssid_val, new_val);
                tick_value_change_obj = NULL;
            }
        } else {
            lv_obj_add_flag(objects.label_ssid_val, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.label_ssid, LV_OBJ_FLAG_HIDDEN);
        }
    }
    // WiFi password
    {
        //const char *cur_val = (objects.label_pass_val) ? lv_label_get_text(objects.label_pass_val) : NULL;
        static const char *lastPassword;

        bool show = get_var_wifiConnected();
            if (show) {
                if (!lv_obj_has_flag(objects.label_pass, LV_OBJ_FLAG_HIDDEN)) {
                    lv_obj_add_flag(objects.label_pass_val, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(objects.label_pass, LV_OBJ_FLAG_HIDDEN);
                }
            } else {
                if (wifi_enabled || local_wifi_enabled) {
                    const char *password = get_var_password();
                    if (lastPassword != password) 
                        {
                            tick_value_change_obj = objects.label_pass_val;
                            LV_LOG_INFO("updating password");
                            lv_label_set_text(objects.label_pass_val, password);
                            tick_value_change_obj = NULL;
                            lastPassword = password;
                        }
                    lv_obj_clear_flag(objects.label_pass_val, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(objects.label_pass, LV_OBJ_FLAG_HIDDEN);
                    }
                else {
                    lv_obj_add_flag(objects.label_pass, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(objects.label_pass_val, LV_OBJ_FLAG_HIDDEN);
                }
            }
    }
    // IP address
    {
        const char *new_val = get_var_ipAddress();
        const char *cur_val = (objects.label_ip_val) ? lv_label_get_text(objects.label_ip_val) : NULL;
        if (new_val == NULL) new_val = "";
    
        if (wifi_enabled) {
            if (strcmp(new_val, cur_val) != 0) {
                tick_value_change_obj = objects.label_ip_val;
                LV_LOG_INFO("updating IP address");
                lv_obj_clear_flag(objects.label_ip_val, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.label_ip, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(objects.label_ip_val, new_val);
                tick_value_change_obj = NULL;
            }
        } else {
            lv_obj_add_flag(objects.label_ip_val, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.label_ip, LV_OBJ_FLAG_HIDDEN);
        }
    }
    // WiFi enable/disable
    {
        if (!objects.switch_wifi) return;

        //bool wifi_enabled = get_var_wifiEnabled();
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

        //bool v1cle_present = get_var_v1clePresent();
        bool usev1cle = get_var_usev1cle();
        bool switch_checked = lv_obj_has_state(objects.switch_v1cle, LV_STATE_CHECKED);

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
        else {
            lv_obj_clear_state(objects.switch_v1cle, LV_STATE_CHECKED);
        }
    }
    // Proxy enable/disable
    {
        if (!objects.switch_proxy) return;

        //bool v1cle_present = get_var_v1clePresent();
        bool useProxy = get_var_useProxy();
        bool switch_checked = lv_obj_has_state(objects.switch_proxy, LV_STATE_CHECKED);

        if (useProxy && !switch_checked) {
            lv_obj_clear_flag(objects.label_proxy, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.switch_proxy, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(objects.switch_proxy, LV_STATE_CHECKED);
        }
        else if (useProxy && switch_checked) {
            lv_obj_clear_flag(objects.label_proxy, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.switch_proxy, LV_OBJ_FLAG_HIDDEN);
            //lv_obj_clear_state(objects.switch_v1cle, LV_STATE_CHECKED);
        }
        else {
            lv_obj_clear_state(objects.switch_proxy, LV_STATE_CHECKED);
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

    lv_obj_set_scroll_dir(obj, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_AUTO);

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
        int current = lv_dropdown_get_selected(objects.unit_of_speed_dropdown);
        int desired = useImperial ? 0 : 1;
    
        if (current != desired) {
            LV_LOG_INFO("setting unit of speed");
            lv_dropdown_set_selected(objects.unit_of_speed_dropdown, desired);
        }
    }
    // Populate SilentRide threshold
    {
        static int last_threshold = -1;
        int threshold = get_var_speedThreshold();

        if (threshold == last_threshold) {
            return;
        }

        LV_LOG_INFO("setting silentRide threshold");
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
        last_threshold = threshold;
    }
    // Show or hide the BT icon setting
    {
        bool showBT = get_var_dispBTIcon();
        bool switch_checked = lv_obj_has_state(objects.show_bt_button, LV_STATE_CHECKED);

        if (showBT != switch_checked) {
            LV_LOG_INFO("updating show BT icon");
            if (showBT) {
                lv_obj_add_state(objects.show_bt_button, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(objects.show_bt_button, LV_STATE_CHECKED);
            }
        }
    }
    // Blank V1 Display
    {
        bool showBlankDisp = get_var_blankDisplay();
        bool switch_checked = lv_obj_has_state(objects.blankscreen_button, LV_STATE_CHECKED);

        if (showBlankDisp != switch_checked) {
            LV_LOG_INFO("updating blank v1 display setting");
            if (showBlankDisp) {
                lv_obj_add_state(objects.blankscreen_button, LV_STATE_CHECKED);
                lv_obj_clear_flag(objects.show_bt_label, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(objects.show_bt_button, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_clear_state(objects.blankscreen_button, LV_STATE_CHECKED);
            }
        }
    }
    // Mute to Gray
    {
        bool muteToGray = get_var_muteToGray();
        bool switch_checked = lv_obj_has_state(objects.mutetogray_button, LV_STATE_CHECKED);
    
        if (muteToGray != switch_checked) {
            LV_LOG_INFO("setting mute to gray");
            if (muteToGray) {
                lv_obj_add_state(objects.mutetogray_button, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(objects.mutetogray_button, LV_STATE_CHECKED);
            }
        }
    }
    // Bogey Counter
    {
        bool showBogeyCounter = get_var_showBogeys();
        bool switch_checked = lv_obj_has_state(objects.showbogeys_button, LV_STATE_CHECKED);
    
        if (showBogeyCounter != switch_checked) {
            LV_LOG_INFO("setting bogey counter");
            if (showBogeyCounter) {
                lv_obj_add_state(objects.showbogeys_button, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(objects.showbogeys_button, LV_STATE_CHECKED);
            }
        }
    }
    // Color Bars
    {
        bool colorBars = get_var_colorBars();
        bool switch_checked = lv_obj_has_state(objects.colorbars_button, LV_STATE_CHECKED);
    
        if (colorBars != switch_checked) {
            LV_LOG_INFO("setting color bars");
            if (colorBars) {
                lv_obj_add_state(objects.colorbars_button, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(objects.colorbars_button, LV_STATE_CHECKED);
            }
        }
    }
    // Default Mode
    {
        bool defaultMode = get_var_useDefaultV1Mode();
        bool switch_checked = lv_obj_has_state(objects.defaultmode_button, LV_STATE_CHECKED);
    
        if (defaultMode != switch_checked) {
            LV_LOG_INFO("setting default mode");
            if (defaultMode) {
                lv_obj_add_state(objects.defaultmode_button, LV_STATE_CHECKED);
            } else {
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
