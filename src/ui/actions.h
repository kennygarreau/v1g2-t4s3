#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

static void blink_timeout_cb(lv_timer_t *timer);
extern void enable_blinking(int index);

#define MAX_BLINK_IMAGES 7
#define MAX_BARS 6
#define MAX_ALERT_ROWS 4
#define BLINK_FREQUENCY 96
#define BLINK_DURATION_MS BLINK_FREQUENCY * 8

extern lv_obj_t *blink_images[MAX_BLINK_IMAGES];
extern bool blink_enabled[MAX_BLINK_IMAGES];
extern int blink_count;
void wifi_switch_event_handler(lv_event_t * e);
void gesture_event_handler(lv_event_t * e);
void slider_brightness_event_handler(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/