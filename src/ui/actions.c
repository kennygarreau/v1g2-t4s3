#include "actions.h"
#include "fonts.h"
#include "images.h"
#include "../tft_v2.h"
#include "screens.h"
#include "ui.h"

static void blink_timeout_cb(lv_timer_t *timer) {
    int index = (int)timer->user_data;
    blink_enabled[index] = false;
    lv_timer_del(timer);
}

static void blink_toggle_cb(lv_timer_t *timer) {
    int index = (int)timer->user_data;

    if (!blink_enabled[index]) {
        lv_timer_del(timer);
        return;
    }

    lv_obj_t *obj = blink_images[index];
    if (obj) {
        bool is_hidden = lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
        if (is_hidden) {
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void disable_blinking(int index) {
    blink_enabled[index] = false;  // The timer will check this and stop itself
    lv_obj_add_flag(blink_images[index], LV_OBJ_FLAG_HIDDEN);
}

void enable_blinking(int index) {
    blink_enabled[index] = true;
    lv_timer_create(blink_timeout_cb, BLINK_DURATION_MS, (void*)index);
}

void wifi_switch_event_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (switch_state) {
        show_popup("WiFi scanning...");
    } else {
        show_popup("Disabling WiFi");
    }

    LV_LOG_INFO("User toggled WiFi switch. New state: %d", switch_state);
    set_var_wifiEnabled(switch_state);
}

void v1cle_switch_event_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (switch_state) {
        show_popup("Enabling V1 CLE");
    } else {
        show_popup("Disabling V1 CLE");
    }
    set_var_usev1cle(switch_state);
    LV_LOG_INFO("User toggled V1 CLE switch. New state: %d", switch_state);
}

void gesture_event_handler(lv_event_t * e) {
    lv_dir_t gesture = lv_indev_get_gesture_dir(lv_indev_get_act());

    if (gesture == LV_DIR_LEFT) {
        LV_LOG_INFO("Swipe Left - Go to Settings");
        lv_scr_load_anim(objects.settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, false);
        loadScreen(SCREEN_ID_SETTINGS);
    } 
    else if (gesture == LV_DIR_RIGHT) {
        LV_LOG_INFO("Swipe Right - Go to Main Screen");
        lv_scr_load_anim(objects.main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, false);
        loadScreen(SCREEN_ID_MAIN);
    }
}

void slider_brightness_event_handler(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int brightness_value = lv_slider_get_value(slider);

    set_var_brightness(brightness_value);

    lv_indev_reset(lv_indev_get_act(), slider);
}

static void band_update_timer(lv_timer_t * timer) {
    if (activeBands & 0b00000001) { // Laser
        lv_obj_clear_flag(objects.band_laser, LV_OBJ_FLAG_HIDDEN);
    }
    if (activeBands & 0b00000010) {  // Ka band
        lv_obj_clear_flag(objects.band_ka, LV_OBJ_FLAG_HIDDEN);
    }
    if (activeBands & 0b00000100) {  // K band
        lv_obj_clear_flag(objects.band_k, LV_OBJ_FLAG_HIDDEN);
    }
    if (activeBands & 0b00001000) { // X band
        lv_obj_clear_flag(objects.band_x, LV_OBJ_FLAG_HIDDEN);
    }
}

void start_band_update_timer() {
    lv_timer_create(band_update_timer, 500, NULL); // Check every 500ms
}

static void clear_inactive_bands_timer(lv_timer_t * timer) {
    activeBands = 0;
    lv_timer_del(timer);
}

void start_clear_inactive_bands_timer() {
    lv_timer_create(clear_inactive_bands_timer, 1000, NULL);
}

void delete_popup(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);

    if (obj == NULL) {
        LV_LOG_USER("delete_popup: NULL object, skipping deletion.");
        return;
    }

    LV_LOG_USER("Deleting popup object...");
}

void show_popup(const char * message) {
    static lv_obj_t * mbox = NULL;

    if (mbox != NULL && lv_obj_is_valid(mbox)) {
        LV_LOG_USER("Popup exists, deleting...");
        lv_obj_del(mbox);
        mbox = NULL;
    }

    mbox = lv_obj_create(lv_scr_act());
    lv_obj_set_size(mbox, 320, 120);
    lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * label = lv_label_create(mbox);
    lv_label_set_text(label, message);
    lv_obj_set_style_text_font(label, &ui_font_alarmclock_32, LV_PART_MAIN | LV_STATE_DEFAULT);
    //lv_obj_set_style_text_color(label, lv_color_hex(0xffff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);

    lv_obj_add_event_cb(mbox, delete_popup, LV_EVENT_DELETE, NULL);
    lv_obj_del_delayed(mbox, 1500);
}

void update_alert_display(bool muted) {
    static bool last_muted_state = false;
    if (muted == last_muted_state) return;

    lv_color_t text_color = muted ? lv_color_hex(gray_color) : lv_color_hex(default_color);
    const void* front_arrow_src = muted ? &img_arrow_front_gray : &img_arrow_front;
    const void* side_arrow_src = muted ? &img_arrow_side_gray : &img_arrow_side;
    const void* rear_arrow_src = muted ? &img_arrow_rear_gray : &img_arrow_rear;

    //lv_obj_set_style_text_color(objects.default_mode, text_color, LV_PART_MAIN | LV_STATE_DEFAULT); // use if testing
    lv_obj_set_style_text_color(objects.prioalertfreq, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.band_k, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.band_ka, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.band_x, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.default_mode, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.custom_freq_en, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_img_set_src(objects.front_arrow, front_arrow_src);
    lv_img_set_src(objects.side_arrow, side_arrow_src);
    lv_img_set_src(objects.rear_arrow, rear_arrow_src);

    last_muted_state = muted;
}

lv_img_dsc_t *allocate_image_in_psram(const lv_img_dsc_t *src_img) {
    if (!src_img) return NULL;

    lv_img_dsc_t *psram_img = (lv_img_dsc_t *)lv_mem_alloc(sizeof(lv_img_dsc_t));
    if (!psram_img) {
        LV_LOG_ERROR("Failed to allocate image descriptor in PSRAM");
        return (lv_img_dsc_t *)src_img;
    }

    memcpy(psram_img, src_img, sizeof(lv_img_dsc_t));  // Copy descriptor

    psram_img->data = (const uint8_t *)lv_mem_alloc(psram_img->data_size);
    if (!psram_img->data) {
        LV_LOG_ERROR("Failed to allocate image data in PSRAM");
        free(psram_img);
        return (lv_img_dsc_t *)src_img;
    }

    memcpy((void *)psram_img->data, src_img->data, psram_img->data_size);

    LV_LOG_INFO("Image allocated in PSRAM: %d bytes\n", psram_img->data_size);
    return psram_img;
}

uint32_t get_bar_color(int i) {
    switch (i) {
        case 0: return green_bar;
        case 1: return yellow_bar;
        case 2: return yellow_bar;
        case 3: return orange_bar;
        case 4: return default_color;
        case 5: return default_color;
        case 6: return default_color;
        default: return default_color;
    }
}