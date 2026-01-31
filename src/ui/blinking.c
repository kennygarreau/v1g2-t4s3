#include "lvgl.h"
#include "blinking.h"
#include "screens.h"
#include "../utils.h"

lv_obj_t *blink_images[MAX_BLINK_IMAGES];  // Store elements to blink
bool blink_enabled[MAX_BLINK_IMAGES] = {false}; // Track which element should blink
uint8_t blink_count = 0;
uint8_t cur_bars = 0;

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
    lv_timer_create(clear_inactive_bands_timer, INACTIVE_BAND_TIMEOUT, NULL);
}

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
    if (index < 0 || index >= MAX_BLINK_IMAGES) { 
        return; 
    }
    lv_obj_t * obj = blink_images[index];
    
    if (obj == NULL) {
        return;
    }

    blink_enabled[index] = false;  // The timer will check this and stop itself
    lv_obj_add_flag(blink_images[index], LV_OBJ_FLAG_HIDDEN);
}

void enable_blinking(int index) {
    blink_enabled[index] = true;
    lv_timer_create(blink_timeout_cb, BLINK_DURATION_MS, (void*)index);
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