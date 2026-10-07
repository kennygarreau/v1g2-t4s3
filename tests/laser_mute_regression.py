"""Verify band-status handling never overrides the detector's mute state."""
from pathlib import Path
import subprocess
import tempfile

s = Path('src/v1_packet.cpp').read_text()
code = s[s.index('void compareBandArrows('):s.index('/* \nExecute if we successfully write reqStartAlertData')]
prefix = r'''
#include <cassert>
#include <cstdio>
struct BandArrowData {bool laser=false,ka=false,k=false,x=false,muteIndicator=false,front=false,side=false,rear=false;};
struct BandState {bool active=false;} ka_state,k_state,x_state,laser_state,front_state,side_state,rear_state;
unsigned char activeBands=0;
bool muted=false;
int unmuteWrites=0;
enum {BLINK_KA,BLINK_K,BLINK_X,BLINK_LASER,BLINK_FRONT,BLINK_SIDE,BLINK_REAR};
void enable_blinking(int){}
void updateBandActivity(bool ka,bool k,bool x,bool laser){ka_state.active=ka;k_state.active=k;x_state.active=x;laser_state.active=laser;}
void updateArrowActivity(bool f,bool s,bool r){front_state.active=f;side_state.active=s;rear_state.active=r;}
void set_var_muted(bool v){muted=v;}
void set_var_prio_alert_freq(const char*){}
void set_var_prioBars(int){}
void reqMuteOff(){++unmuteWrites;}
'''
suffix = r'''
int main(){
 for(int type=0;type<5;++type){
  BandArrowData status,previous;status.front=true;
  status.laser=type==0||type==4;status.ka=type==1||type==4;
  status.k=type==2;status.x=type==3;
  for(int cycle=0;cycle<100;++cycle){
   status.muteIndicator=true;compareBandArrows(status,previous);assert(muted);
   status.muteIndicator=false;compareBandArrows(status,previous);assert(!muted);
   assert(unmuteWrites==0);
  }
 }
 puts("PASS: laser, Ka, K, X and mixed status preserve detector mute without unsolicited writes");
}
'''
with tempfile.TemporaryDirectory(prefix='v1-laser-mute-') as tmp:
    d = Path(tmp)
    (d/'test.cpp').write_text(prefix + code + suffix)
    subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined',str(d/'test.cpp'),'-o',str(d/'test')],check=True)
    subprocess.run([str(d/'test')],check=True)
