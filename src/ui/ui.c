#if defined(EEZ_FOR_LVGL)
#include <eez/core/vars.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"

#ifdef __cplusplus
}
#endif


#if defined(EEZ_FOR_LVGL)

void ui_init() {
    eez_flow_init(assets, sizeof(assets), (lv_obj_t **)&objects, sizeof(objects), images, sizeof(images), actions);
}

void ui_tick() {
    eez_flow_tick();
    tick_screen(g_currentScreen);
}

#else

#include <string.h>

int16_t currentScreen = -1;

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    if (index == -1) {
        return 0;
    }
    return ((lv_obj_t **)&objects)[index];
}

void loadScreen(enum ScreensEnum screenId) {
    loadScreenAnimated(screenId, LV_SCR_LOAD_ANIM_FADE_IN);
}

void loadScreenAnimated(enum ScreensEnum screenId, lv_scr_load_anim_t animation) {
    currentScreen = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromIndex(currentScreen);
    lv_scr_load_anim(screen, animation, 220, 0, false);
}
void ui_boot() {
    create_screen_logo_screen();
    loadScreen(SCREEN_ID_LOGO_SCREEN);
}

void ui_init() {
    create_screens();
    loadScreen(SCREEN_ID_MAIN);
    //start_band_update_timer();
    //start_clear_inactive_bands_timer();
}

void ui_tick() {
    tick_screen(currentScreen);
}

void ui_tick_statusBar() {
    tick_status_bar();
}

#endif
