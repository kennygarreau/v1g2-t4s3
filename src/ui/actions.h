#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BLINK_IMAGES 7
#define MAX_BARS 6
#define MAX_ALERT_ROWS 4
#define BLINK_FREQUENCY 96
#define BLINK_DURATION_MS BLINK_FREQUENCY * 8

static void blink_timeout_cb(lv_timer_t *timer);
extern void enable_blinking(int index);
extern void disable_blinking(int index);

static void clear_inactive_bands_timer(lv_timer_t * timer);
void start_clear_inactive_bands_timer();
void start_band_update_timer();

extern lv_obj_t *blink_images[MAX_BLINK_IMAGES];
extern bool blink_enabled[MAX_BLINK_IMAGES];
extern int blink_count;

void main_press_handler(lv_event_t * e);
void v1cle_switch_event_handler(lv_event_t * e);
void wifi_switch_event_handler(lv_event_t * e);
void gesture_event_handler(lv_event_t * e);
void slider_brightness_event_handler(lv_event_t * e);
void show_popup(const char * message);
void update_alert_display(bool muted);
uint32_t get_bar_color(int i);

extern lv_img_dsc_t *allocate_image_in_psram(const lv_img_dsc_t *src_img);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/