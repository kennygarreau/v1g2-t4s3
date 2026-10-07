"""Host tests of actual touch reader and gesture handler, not physical input."""
from pathlib import Path
import subprocess
import tempfile
import sys

touch_source = Path(sys.argv[1] if len(sys.argv) > 1 else "src/LV_Helper.cpp").read_text()
touch = touch_source.split("static void touchpad_read(", 1)[1].split("\n#ifndef BOARD_HAS_PSRAM", 1)[0]
touch = "static void touchpad_read(" + touch
actions = Path("src/ui/actions.c").read_text()
gesture = "void gesture_event_handler(" + actions.split("void gesture_event_handler(", 1)[1].split("void slider_brightness_event_handler(", 1)[0]
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
bool touchInterrupt = false;
enum { LV_INDEV_STATE_REL, LV_INDEV_STATE_PR };
struct lv_indev_drv_t {void* user_data;};
struct lv_indev_data_t {struct {int16_t x,y;} point; int state;};
struct LilyGo_Display {
  bool held = true;
  uint8_t getPoint(int16_t* x,int16_t* y,int) {*x=120;*y=70;return held;}
};
struct lv_event_t {};
struct lv_indev_t {};
struct lv_obj_t {int id;};
enum lv_dir_t {LV_DIR_NONE, LV_DIR_LEFT, LV_DIR_RIGHT};
enum {SCREEN_ID_MAIN=1, SCREEN_ID_SETTINGS=2, SCREEN_ID_DISPSETTINGS=3};
enum {LV_SCR_LOAD_ANIM_MOVE_LEFT, LV_SCR_LOAD_ANIM_MOVE_RIGHT};
lv_obj_t main_obj{1}, settings_obj{2}, display_obj{3};
struct {lv_obj_t* main=&main_obj;lv_obj_t* settings=&settings_obj;lv_obj_t* dispSettings=&display_obj;} objects;
lv_obj_t* active=&main_obj;
lv_indev_t device;
lv_indev_t* input=&device;
lv_dir_t direction=LV_DIR_NONE;
int waits=0, loads=0;
lv_indev_t* lv_indev_get_act(){return input;}
lv_obj_t* lv_scr_act(){return active;}
lv_dir_t lv_indev_get_gesture_dir(lv_indev_t*){return direction;}
void lv_indev_wait_release(lv_indev_t*){++waits;}
void loadScreen(int id){++loads;active=id==1?objects.main:id==2?objects.settings:objects.dispSettings;}
void loadScreenAnimated(int id,int animation){assert(animation==(direction==LV_DIR_LEFT?LV_SCR_LOAD_ANIM_MOVE_LEFT:LV_SCR_LOAD_ANIM_MOVE_RIGHT));loadScreen(id);}
#define LV_LOG_INFO(...) ((void)0)
'''
suffix = r'''
int main(){
  LilyGo_Display display;
  lv_indev_drv_t driver{&display};lv_indev_data_t state{};
  for(int i=0;i<50;++i){touchInterrupt=(i==0);touchpad_read(&driver,&state);assert(state.state==LV_INDEV_STATE_PR);}
  display.held=false;touchpad_read(&driver,&state);assert(state.state==LV_INDEV_STATE_REL);
  lv_event_t event;
  for(int i=0;i<20;++i){
    direction=LV_DIR_LEFT;gesture_event_handler(&event);assert(active==objects.settings);
    direction=LV_DIR_LEFT;gesture_event_handler(&event);assert(active==objects.dispSettings);
    direction=LV_DIR_RIGHT;gesture_event_handler(&event);assert(active==objects.settings);
    direction=LV_DIR_RIGHT;gesture_event_handler(&event);assert(active==objects.main);
  }
  assert(waits==80 && loads==80);
  input=nullptr;gesture_event_handler(&event);assert(loads==80);
  puts("PASS: held contact across IRQ gaps, real release, navigation, and null input");
}
'''
with tempfile.TemporaryDirectory(prefix="v1-touch-test-") as tmp:
    path=Path(tmp)
    (path/"test.cpp").write_text(prefix+touch+gesture+suffix)
    subprocess.run(["clang++","-std=c++17","-fsanitize=address,undefined",str(path/"test.cpp"),"-o",str(path/"test")],check=True)
    result=subprocess.run([str(path/"test")],capture_output=True,text=True)
    if result.returncode:
        print("FAIL:",result.stderr.splitlines()[0] if result.stderr else result.returncode)
        sys.exit(1)
    print(result.stdout.strip())
