#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BARS 6
#define MAX_ALERT_ROWS 4

void main_press_handler(lv_event_t * e);
void v1cle_switch_event_handler(lv_event_t * e);
void proxy_switch_event_handler(lv_event_t * e);
void wifi_switch_event_handler(lv_event_t * e);
void muteToGray_handler(lv_event_t * e);
void colorBars_handler(lv_event_t * e);
void defaultMode_handler(lv_event_t * e);
void bogeyCounter_handler(lv_event_t * e);
void blankV1display_handler(lv_event_t * e);
void showBT_handler(lv_event_t * e);
void gesture_event_handler(lv_event_t * e);
void slider_brightness_event_handler(lv_event_t * e);
void unitOfSpeed_handler(lv_event_t * e);
void speedThreshold_handler(lv_event_t * e);
void show_popup(const char * message);
void update_alert_display(bool muted);
uint32_t get_bar_color(int i);
static void hide_after_animation_cb(lv_anim_t * a);
void fade_out_and_hide(lv_obj_t * obj, uint32_t delay);

extern lv_img_dsc_t *allocate_image_in_psram(const lv_img_dsc_t *src_img);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/