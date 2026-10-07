"""Exercise actual main input handler with host-side BLE/LVGL collaborators."""
from pathlib import Path
import subprocess
import tempfile

s=Path("src/utils.cpp").read_text()
handler='extern "C" void main_press_handler('+s.split('extern "C" void main_press_handler(',1)[1].split('extern "C" bool get_var_alertPresent()',1)[0]
feedback=s.split('static bool unmutePending',1)[1].split('//int locationCount',1)[0]
feedback='static bool unmutePending'+feedback
prefix=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <atomic>
enum lv_event_code_t {LV_EVENT_LONG_PRESSED,LV_EVENT_CLICKED,LV_EVENT_SHORT_CLICKED,LV_EVENT_RELEASED};
struct lv_event_t {lv_event_code_t code;};
struct lv_indev_t {};
lv_indev_t input;
lv_event_code_t lv_event_get_code(lv_event_t* e){return e->code;}
lv_indev_t* lv_indev_get_act(){return &input;}
enum lv_dir_t {LV_DIR_NONE,LV_DIR_LEFT,LV_DIR_RIGHT};
lv_dir_t lv_indev_get_gesture_dir(lv_indev_t*){return LV_DIR_NONE;}
enum {MODE_ALL_BOGEYS=1,MODE_LOGIC=2,MODE_ADVLOGIC=3};
enum {SCREEN_ID_SETTINGS=1,SCREEN_ID_MAIN=2,LV_SCR_LOAD_ANIM_MOVE_LEFT,LV_SCR_LOAD_ANIM_MOVE_RIGHT};
struct {int settings,main;} objects;
void lv_scr_load_anim(int,int,int,int,bool){}
void loadScreen(int){}
struct {uint8_t rawMode=1;} globalConfig;
struct {bool displayTest=false;} settings;
bool bt_connected=true,alertPresent=true,muted=true,needsMode=false;
int muteWrites=0,unmuteWrites=0,modeWrites=0,popups=0;
bool writeSucceeds=true;
unsigned long now=0;
unsigned long millis(){return now;}
struct Packet {
  static uint8_t* reqMuteOn(){static uint8_t p[7]={1};return p;}
  static uint8_t* reqMuteOff(){static uint8_t p[7]={2};return p;}
  static uint8_t* reqChangeMode(uint8_t){static uint8_t p[8]={3};return p;}
};
struct BLE {
  bool writeValue(uint8_t* p,int,bool){if(p[0]==1)++muteWrites;else if(p[0]==2)++unmuteWrites;else ++modeWrites;return writeSucceeds;}
} ble;
BLE* clientWriteCharacteristic=&ble;
struct {void println(const char*){} template<class...T> void printf(const char*,T...){} } Serial;
void show_popup(const char*){++popups;}
void delay(int){}
#define LV_LOG_INFO(...) ((void)0)
#define LV_LOG_WARN(...) ((void)0)
'''
suffix=r'''
void event(lv_event_code_t c){lv_event_t e{c};main_press_handler(&e);}
int main(){
  detectorReportedMuted=true;++muteStatusRevision;
  event(LV_EVENT_LONG_PRESSED);assert(unmuteWrites==1&&muteWrites==0&&modeWrites==0&&popups==0);
  processMuteFeedback();assert(popups==0);
  muted=false;processMuteFeedback();assert(popups==0); // Table/local state is not an acknowledgement.
  detectorReportedMuted=false;++muteStatusRevision;processMuteFeedback();assert(popups==1);
  processMuteFeedback();assert(popups==1);muted=true;
  event(LV_EVENT_RELEASED);event(LV_EVENT_CLICKED);assert(muteWrites==0);
  event(LV_EVENT_SHORT_CLICKED);assert(muteWrites==1);
  muted=false;detectorReportedMuted=false;event(LV_EVENT_LONG_PRESSED);assert(unmuteWrites==1&&modeWrites==0);
  event(LV_EVENT_RELEASED);
  muted=true;detectorReportedMuted=true;bt_connected=false;event(LV_EVENT_LONG_PRESSED);assert(unmuteWrites==1);
  event(LV_EVENT_RELEASED);bt_connected=true;
  settings.displayTest=true;event(LV_EVENT_LONG_PRESSED);assert(unmuteWrites==1);
  event(LV_EVENT_RELEASED);settings.displayTest=false;
  writeSucceeds=false;int oldPopups=popups;event(LV_EVENT_LONG_PRESSED);assert(popups==oldPopups);
  event(LV_EVENT_RELEASED);writeSucceeds=true;
  event(LV_EVENT_LONG_PRESSED);now=3000;processMuteFeedback();assert(!unmutePending);
  event(LV_EVENT_RELEASED);
  alertPresent=false;event(LV_EVENT_LONG_PRESSED);assert(modeWrites==1&&needsMode);
  puts("PASS: unmute, no release remute, short tap, idle mode, and write guards");
}
'''
with tempfile.TemporaryDirectory(prefix="v1-long-press-") as tmp:
    p=Path(tmp);(p/"test.cpp").write_text(prefix+feedback+handler+suffix)
    subprocess.run(["clang++","-std=c++17","-fsanitize=address,undefined",str(p/"test.cpp"),"-o",str(p/"test")],check=True)
    subprocess.run([str(p/"test")],check=True)
