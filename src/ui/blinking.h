#ifndef BLINKING_H
#define BLINKING_H

#ifdef __cplusplus
extern "C" {
#endif

#define BLINK_FREQUENCY 96
#define BLINK_DURATION_MS BLINK_FREQUENCY * 12
#define INACTIVE_BAND_TIMEOUT 1200
#define MAX_BLINK_IMAGES 7

enum BlinkIndices {
    BLINK_FRONT = 0,
    BLINK_SIDE = 1,
    BLINK_REAR = 2,
    BLINK_X = 3,
    BLINK_K = 4,
    BLINK_KA = 5,
    BLINK_LASER = 6
};

extern lv_obj_t *blink_images[MAX_BLINK_IMAGES];
extern bool blink_enabled[MAX_BLINK_IMAGES];
extern uint8_t blink_count;

static void blink_timeout_cb(lv_timer_t *timer);
extern void enable_blinking(int index);
extern void disable_blinking(int index);

static void clear_inactive_bands_timer(lv_timer_t * timer);
void start_clear_inactive_bands_timer();
void start_band_update_timer();

void register_blinking_image(int index, lv_obj_t *obj);
void init_blinking_system(void);

#ifdef __cplusplus
}
#endif

#endif