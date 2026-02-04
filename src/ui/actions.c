#include "actions.h"
#include "fonts.h"
#include "images.h"
#include "../utils.h"
#include "blinking.h"
#include "screens.h"
#include "ui.h"
#include "esp_heap_caps.h"

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

void proxy_switch_event_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (switch_state) {
        show_popup("Enabling Proxy");
    } else {
        show_popup("Disabling Proxy");
    }
    set_var_useProxy(switch_state);
    LV_LOG_INFO("User toggled Proxy switch. New state: %d", switch_state);
}

void muteToGray_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (switch_state) {
        show_popup("Enable Mute-to-Gray");
    } else {
        show_popup("Disable Mute-to-Gray");
    }
    set_var_muteToGray(switch_state);
    LV_LOG_INFO("User toggled Mute-to-Gray switch. New state: %d", switch_state);
}

void colorBars_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (switch_state) {
        show_popup("Enable Color Bars");
    } else {
        show_popup("Disable Color Bars");
    }
    set_var_colorBars(switch_state);
    LV_LOG_INFO("User toggled Color Bars switch. New state: %d", switch_state);
}

void defaultMode_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (switch_state) {
        show_popup("Enabling Default Mode");
    } else {
        show_popup("Disabling Default Mode");
    }
    set_var_useDefaultV1Mode(switch_state);
    LV_LOG_INFO("User toggled Default Mode switch. New state: %d", switch_state);
}

void bogeyCounter_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (switch_state) {
        show_popup("Showing Bogeys");
    } else {
        show_popup("Hiding Bogeys");
    }
    set_var_showBogeys(switch_state);
    LV_LOG_INFO("User toggled Show Bogeys switch. New state: %d", switch_state);
}

void blankV1display_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (switch_state) {
        show_popup("Blanking Display");
    } else {
        show_popup("Showing Display");
    }
    set_var_blankDisplay(switch_state);
    LV_LOG_INFO("User toggled V1 Display switch. New state: %d", switch_state);
}

void showBT_handler(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool switch_state = lv_obj_has_state(obj, LV_STATE_CHECKED);

    if (switch_state) {
        show_popup("Showing BT Icon");
    } else {
        show_popup("Hiding BT Icon");
    }
    set_var_dispBTIcon(switch_state);
    LV_LOG_INFO("User toggled BT Icon switch. New state: %d", switch_state);
}

void speedThreshold_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        uint16_t selected = lv_dropdown_get_selected(obj);
        int threshold = 0;

        switch (selected) {
            case 0: threshold = 20; break;
            case 1: threshold = 25; break;
            case 2: threshold = 30; break;
            case 3: threshold = 35; break;
            case 4: threshold = 40; break;
            case 5: threshold = 45; break;
            case 6: threshold = 50; break;
            case 7: threshold = 55; break;
            case 8: threshold = 60; break;
            default: return;
        }

        set_var_speedThreshold(threshold);
    }
}

void unitOfSpeed_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        uint16_t selected = lv_dropdown_get_selected(obj);
        bool value;

        switch (selected) {
            case 0: value = true; break;
            case 1: value = false; break;
            default: return;
        }

        set_var_useImperial(value);
    }
}

void gesture_event_handler(lv_event_t * e) {
    lv_obj_t *screen = lv_scr_act();
    lv_dir_t gesture = lv_indev_get_gesture_dir(lv_indev_get_act());

    if (gesture == LV_DIR_LEFT) {
        if (screen == objects.main) {
            LV_LOG_INFO("Swipe Left - Main -> Settings");
            lv_scr_load_anim(objects.settings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, false);
            loadScreen(SCREEN_ID_SETTINGS);
        } else if (screen == objects.settings) {
            LV_LOG_INFO("Swipe Left - Settings -> Display Settings");
            lv_scr_load_anim(objects.dispSettings, LV_SCR_LOAD_ANIM_MOVE_LEFT, 100, 0, false);
            loadScreen(SCREEN_ID_DISPSETTINGS);
        }
    } 
    else if (gesture == LV_DIR_RIGHT) {
        if (screen == objects.settings) {
            LV_LOG_INFO("Swipe Right - Settings -> Main");
            lv_scr_load_anim(objects.main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, false);
            loadScreen(SCREEN_ID_MAIN);
        } else if (screen == objects.dispSettings) {
            LV_LOG_INFO("Swipe Right - Display Settings -> Settings");
            lv_scr_load_anim(objects.settings, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 100, 0, false);
            loadScreen(SCREEN_ID_SETTINGS);
        }
    }
}

void slider_brightness_event_handler(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int brightness_value = lv_slider_get_value(slider);

    set_var_brightness(brightness_value);

    lv_indev_reset(lv_indev_get_act(), slider);
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
    lv_obj_set_size(mbox, 420, 120);
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

    lv_color_t text_color = muted ? lv_color_hex(gray_color) : lv_color_hex(default_color);
    const void* front_arrow_src = muted ? &img_arrow_front_gray_4bit : &img_arrow_front;
    const void* side_arrow_src = muted ? &img_arrow_side_gray_4bit : &img_arrow_side;
    const void* rear_arrow_src = muted ? &img_arrow_rear_gray_4bit : &img_arrow_rear;

    lv_obj_set_style_text_color(objects.prioalertfreq, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.band_k, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.band_ka, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.band_x, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.default_mode, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.bogey_count, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(objects.custom_freq_en, text_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_img_set_src(objects.front_arrow, front_arrow_src);
    lv_img_set_src(objects.side_arrow, side_arrow_src);
    lv_img_set_src(objects.rear_arrow, rear_arrow_src);
    
}

lv_img_dsc_t *allocate_image_in_psram(const lv_img_dsc_t *src_img) {
    if (!src_img) return NULL;

    //lv_img_dsc_t *psram_img = (lv_img_dsc_t *)lv_mem_alloc(sizeof(lv_img_dsc_t));
    lv_img_dsc_t *psram_img = (lv_img_dsc_t *)malloc(sizeof(lv_img_dsc_t));
    if (!psram_img) {
        LV_LOG_ERROR("Failed to allocate image descriptor in PSRAM");
        return (lv_img_dsc_t *)src_img;
    }

    memcpy(psram_img, src_img, sizeof(lv_img_dsc_t));  // Copy descriptor

    //psram_img->data = (const uint8_t *)lv_mem_alloc(psram_img->data_size);
    psram_img->data = (const uint8_t *)heap_caps_aligned_alloc(32, psram_img->data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
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

static void hide_after_animation_cb(lv_anim_t * a) {
    lv_obj_add_flag((lv_obj_t *)a->var, LV_OBJ_FLAG_HIDDEN);
}

void fade_out_and_hide(lv_obj_t * obj, uint32_t delay) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP); // From 255 to 0 (visible to transparent)
    lv_anim_set_time(&a, 500); // 500ms duration
    lv_anim_set_delay(&a, delay);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_ready_cb(&a, hide_after_animation_cb); // Hide when done
    lv_anim_start(&a);
}
