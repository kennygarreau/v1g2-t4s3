#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "tft_v2.h"

#include <string.h>
#include <stdio.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
lv_obj_t* signal_bars[MAX_BARS];
lv_obj_t* alert_rows[MAX_ALERT_ROWS];
lv_obj_t* alert_directions[MAX_ALERT_ROWS];

lv_obj_t *blink_images[MAX_BLINK_IMAGES];  // Stores images to blink
bool blink_enabled[MAX_BLINK_IMAGES] = {false}; // Track which should blink
int blink_count = 0;

static uint32_t last_blink_time = 0;
static bool blink_state = false; // Tracks whether the arrow is currently visible

lv_obj_t* create_alert_row(lv_obj_t* parent, int x, int y, const char* frequency) {
    lv_obj_t* obj = lv_label_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_set_text(obj, frequency);
    lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN); // Start as hidden
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

void update_alert_rows(int num_alerts, const char* frequencies[]) {
    for (int i = 0; i < MAX_ALERT_ROWS; i++) {
        if (i < num_alerts) {
            lv_label_set_text(alert_rows[i], frequencies[i]);
            lv_obj_clear_flag(alert_rows[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(alert_rows[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void update_alert_arrows(int num_alerts, const char* directions[]) {
    for (int i = 0; i < MAX_ALERT_ROWS; i++) {
        if (i < num_alerts) {
            if (strcmp(directions[i], "FRONT") == 0) {
                lv_img_set_src(alert_directions[i], &img_small_arrow_front);
            } else if (strcmp(directions[i], "SIDE") == 0) {
                lv_img_set_src(alert_directions[i], &img_small_arrow_side);
            } else if (strcmp(directions[i], "REAR") == 0) {
                lv_img_set_src(alert_directions[i], &img_small_arrow_rear);
            } else {
                lv_img_set_src(alert_directions[i], &img_small_arrow_front);
            }
            lv_obj_clear_flag(alert_directions[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(alert_directions[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

lv_obj_t* create_signal_bar(lv_obj_t* parent, int x, int y) {
    lv_obj_t* obj = lv_label_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    //lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, "             ");
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
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
    for (int i = 0; i < MAX_BARS; ++i) {
        if (signal_bars[i] == NULL) {
            continue;
        }

        if (i < num_visible) {
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
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // bt_logo
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.bt_logo = obj;
            lv_obj_set_pos(obj, 534, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_bt_logo_small);
            lv_img_set_zoom(obj, 128);
        }
        {
            // mode_type
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.mode_type = obj;
            lv_obj_set_pos(obj, 17, 16);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // mute_logo
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.mute_logo = obj;
            lv_obj_set_pos(obj, 420, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_mute_logo_small);
            lv_img_set_zoom(obj, 128);
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
            lv_img_set_src(obj, &img_arrow_front);
            lv_obj_align(obj, LV_ALIGN_CENTER, -24, -70);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // side arrow
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.side_arrow = obj;
            lv_img_set_src(obj, &img_arrow_side);
            lv_obj_align(obj, LV_ALIGN_CENTER, -24, 0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // rear arrow
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.rear_arrow = obj;
            lv_img_set_src(obj, &img_arrow_rear);
            lv_obj_align(obj, LV_ALIGN_CENTER, -24, 50);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // nav_logo_enabled
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.nav_logo_enabled = obj;
            lv_obj_set_pos(obj, 500, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_location_red);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // nav_logo_disabled
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.nav_logo_disabled = obj;
            lv_obj_set_pos(obj, 500, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_img_set_src(obj, &img_location_disabled);
            lv_img_set_zoom(obj, 128);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // automutespeed
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.automutespeed = obj;
            lv_obj_set_pos(obj, 473, 18);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
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
        {
            // band_laser
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.band_laser = obj;
            lv_obj_set_pos(obj, 14, 108);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // band_ka
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.band_ka = obj;
            lv_obj_set_pos(obj, 14, 146);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // band_k
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.band_k = obj;
            lv_obj_set_pos(obj, 14, 184);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            // band_x
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.band_x = obj;
            lv_obj_set_pos(obj, 14, 222);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, "");
            lv_obj_set_style_text_font(obj, &ui_font_alarmclock_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
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

void tick_screen_main() {
    {
        // Up Arrow
        uint32_t now = lv_tick_get();
        bool should_blink = blink_enabled[BLINK_FRONT];
        if (should_blink) {
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
                tick_value_change_obj = objects.front_arrow;
                if (new_val) lv_obj_clear_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN);
                else lv_obj_add_flag(objects.front_arrow, LV_OBJ_FLAG_HIDDEN);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        // Side Arrow
        uint32_t now = lv_tick_get();
        bool should_blink = blink_enabled[BLINK_SIDE];
        if (should_blink) {
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
                tick_value_change_obj = objects.side_arrow;
                if (new_val) lv_obj_clear_flag(objects.side_arrow, LV_OBJ_FLAG_HIDDEN);
                else lv_obj_add_flag(objects.side_arrow, LV_OBJ_FLAG_HIDDEN);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        // Rear Arrow
        uint32_t now = lv_tick_get();
        bool should_blink = blink_enabled[BLINK_REAR];
        if (should_blink) {
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
                tick_value_change_obj = objects.rear_arrow;
                if (new_val) lv_obj_clear_flag(objects.rear_arrow, LV_OBJ_FLAG_HIDDEN);
                else lv_obj_add_flag(objects.rear_arrow, LV_OBJ_FLAG_HIDDEN);
                tick_value_change_obj = NULL;
            }
        }
    }
    {    
        // Priority Alert Frequency
        const char *new_val = get_var_prio_alert_freq();
        const char *cur_val = lv_label_get_text(objects.prioalertfreq);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.prioalertfreq;
            if (new_val) { lv_label_set_text(objects.prioalertfreq, new_val);
            //lv_obj_clear_flag(objects.prioalertfreq, LV_OBJ_FLAG_HIDDEN); 
            }
            //else lv_obj_add_flag(objects.prioalertfreq, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        } 
    }
    {
        // Priority Alert Signal Bars
        int numBars = get_var_prioBars();
        update_signal_bars(numBars);
    }
    {
        // Alert Table visibility
        bool new_val = get_alertPresent(); // true if alert present
        bool cur_val = lv_obj_has_flag(objects.alert_table, LV_OBJ_FLAG_HIDDEN); // true if hidden
        if (new_val == cur_val) {
            tick_value_change_obj = objects.alert_table;
            if (new_val) lv_obj_clear_flag(objects.alert_table, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(objects.alert_table, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        // Alert Table
        int alertCount = get_var_alertCount();
        const char** frequencies = get_var_frequencies();
        const char** directions = get_var_directions();
        update_alert_rows(alertCount, frequencies);
        update_alert_arrows(alertCount, directions);
    }
    {
        // Bluetooth status
        bool new_val = get_var_bt_connected(); // true when connected
        bool cur_val = lv_obj_has_flag(objects.bt_logo, LV_OBJ_FLAG_HIDDEN); // true if "hidden"
        if (new_val == cur_val) {
            tick_value_change_obj = objects.bt_logo;
            if (new_val) lv_obj_clear_flag(objects.bt_logo, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(objects.bt_logo, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        // Logic Mode
        const char *new_val = get_var_logicmode();
        const char *cur_val = (objects.mode_type) ? lv_label_get_text(objects.mode_type) : NULL;
        if (new_val && cur_val && strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.mode_type;
            lv_label_set_text(objects.mode_type, new_val);
            tick_value_change_obj = NULL;
        } else if (!new_val) {
            LV_LOG_ERROR("Error: get_var_logicmode() returned NULL!");
        } else if (!objects.mode_type) {
            LV_LOG_ERROR("Error: objects.mode_type is NULL!");
        }
    }
    {
        // Mute status
        bool new_val = get_var_muted(); // true if muted
        bool cur_val = lv_obj_has_flag(objects.mute_logo, LV_OBJ_FLAG_HIDDEN); // true if hidden
        if (new_val == cur_val) {
            tick_value_change_obj = objects.mute_logo;
            if (new_val) lv_obj_clear_flag(objects.mute_logo, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(objects.mute_logo, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        // Nav enabled logo
        bool new_val = get_var_gpsEnabled(); // true if enabled
        bool cur_val = lv_obj_has_flag(objects.nav_logo_enabled, LV_OBJ_FLAG_HIDDEN); // true if hidden
        if (new_val == cur_val) {
            tick_value_change_obj = objects.nav_logo_enabled;
            if (new_val) lv_obj_clear_flag(objects.nav_logo_enabled, LV_OBJ_FLAG_HIDDEN); // show the gps enabled logo
            else lv_obj_add_flag(objects.nav_logo_enabled, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        // Nav disabled logo
        bool new_val = get_var_gpsEnabled(); // true if enabled
        bool cur_val = lv_obj_has_flag(objects.nav_logo_disabled, LV_OBJ_FLAG_HIDDEN); // true if hidden
        if (new_val != cur_val) {
            tick_value_change_obj = objects.nav_logo_disabled; 
            if (new_val) lv_obj_clear_flag(objects.nav_logo_disabled, LV_OBJ_FLAG_HIDDEN); // show the gps disabled logo
            else lv_obj_add_flag(objects.nav_logo_disabled, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        // KA alert
        bool new_val = get_var_kaAlert(); // true if enabled
        bool cur_val = lv_obj_has_flag(objects.band_ka, LV_OBJ_FLAG_HIDDEN); // true if hidden
        if (new_val == cur_val) {
            tick_value_change_obj = objects.band_ka;
            if (new_val) { lv_obj_clear_flag(objects.band_ka, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(objects.band_ka, "KA"); }
            else lv_obj_add_flag(objects.band_ka, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        // Laser alert
        bool new_val = get_var_laserAlert(); // true if enabled
        bool cur_val = lv_obj_has_flag(objects.band_laser, LV_OBJ_FLAG_HIDDEN); // true if hidden
        if (new_val == cur_val) {
            tick_value_change_obj = objects.band_laser;
            if (new_val) { lv_obj_clear_flag(objects.band_laser, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(objects.band_laser, "L"); }
            else lv_obj_add_flag(objects.band_laser, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        // K alert
        bool new_val = get_var_kAlert(); // true if enabled
        bool cur_val = lv_obj_has_flag(objects.band_k, LV_OBJ_FLAG_HIDDEN); // true if hidden
        if (new_val == cur_val) {
            tick_value_change_obj = objects.band_k;
            if (new_val) { lv_obj_clear_flag(objects.band_k, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(objects.band_k, "K"); }
            else lv_obj_add_flag(objects.band_k, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        // X alert
        bool new_val = get_var_xAlert(); // true if enabled
        bool cur_val = lv_obj_has_flag(objects.band_x, LV_OBJ_FLAG_HIDDEN); // true if hidden
        if (new_val == cur_val) {
            tick_value_change_obj = objects.band_x;
            if (new_val) { lv_obj_clear_flag(objects.band_x, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(objects.band_x, "X"); }
            else lv_obj_add_flag(objects.band_x, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
    {
        // SilentRide threshold
        const char *new_val = get_var_lowspeedthreshold();
        const char *cur_val = lv_label_get_text(objects.automutespeed);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.automutespeed;
            lv_obj_clear_flag(objects.automutespeed, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(objects.automutespeed, new_val); 
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

void create_screen_settings() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 600, 450);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 600, 52);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            //lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd4d9e2), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 22, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            lv_obj_set_pos(obj, 234, 102);
            lv_obj_set_size(obj, 341, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "Option 1\nOption 2\nOption 3");
        }
    }
}

void tick_screen_settings() {
}


void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_logo_screen();
    create_screen_main();
    create_screen_settings();
}

typedef void (*tick_screen_func_t)();

tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_logo_screen,
    tick_screen_main,
    tick_screen_settings,
};

void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
