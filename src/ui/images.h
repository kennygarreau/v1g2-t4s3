#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_v1gen2logo_white;
extern const lv_img_dsc_t img_bt_logo_small;
extern const lv_img_dsc_t img_bt_proxy;
extern const lv_img_dsc_t img_location_disabled;
extern const lv_img_dsc_t img_mute_logo_small;
extern const lv_img_dsc_t img_location_red;
extern const lv_img_dsc_t img_arrow_front;
extern const lv_img_dsc_t img_arrow_front_gray_4bit;
extern const lv_img_dsc_t img_arrow_side;
extern const lv_img_dsc_t img_arrow_side_gray_4bit;
extern const lv_img_dsc_t img_arrow_rear;
extern const lv_img_dsc_t img_arrow_rear_gray_4bit;
extern const lv_img_dsc_t img_small_arrow_rear;
extern const lv_img_dsc_t img_small_arrow_front;
extern const lv_img_dsc_t img_small_arrow_side;
extern const lv_img_dsc_t img_wifi;
extern const lv_img_dsc_t img_wifi_local;
extern const lv_img_dsc_t img_wifi_localConnected;
//extern const lv_img_dsc_t img_photo_camera;
extern const lv_img_dsc_t ui_image_photo_camera;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[19];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/