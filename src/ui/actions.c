#include "actions.h"
#include "../tft_v2.h"
#include "screens.h"
#include "ui.h"

static void blink_timeout_cb(lv_timer_t *timer) {
    int index = (int)timer->user_data;
    blink_enabled[index] = false;
    lv_timer_del(timer);
}

void enable_blinking(int index) {
    blink_enabled[index] = true;
    lv_timer_create(blink_timeout_cb, BLINK_DURATION_MS, (void*)index);
}

void wifi_switch_event_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    LV_LOG_INFO("User toggled WiFi switch. New state: %d", switch_state);
    set_var_wifiEnabled(switch_state);
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
