"""Exercise actual network save, boot reload and delayed reboot code."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('.')
web = (root/'src/web.cpp').read_text()
boot = (root/'src/v1server.cpp').read_text()
save = web[web.index('            if (doc.containsKey("wifiCredentials")'):web.index('            if (doc.containsKey("localSSID")')]
load = boot[boot.index('  settings.wifiCredentials.clear();'):boot.index('  loadLockoutSettings();')]
reboot = web[web.index('void checkReboot()'):web.index('uint32_t hexToUint32')]
prefix = r'''
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdio>
using std::min;
struct String : std::string {
 using std::string::string;
 String(const std::string &s):std::string(s){}
 String(int n):std::string(std::to_string(n)){}
};
struct SerialMock {template<class T> void println(T){} template<class... T> void printf(T...){} } Serial;
struct Value {String value; template<class T> T as(){return value;}};
struct JsonObject {
 String ssid,password;
 bool containsKey(const char*){return true;}
 Value operator[](const char* k){return {std::string(k)=="ssid"?ssid:password};}
};
struct JsonArray : std::vector<JsonObject>{using std::vector<JsonObject>::vector;};
struct ArrayValue {JsonArray value;template<class T> bool is(){return true;}template<class T> T as(){return value;}};
struct Document {bool present=true;JsonArray networks;bool containsKey(const char*){return present;}ArrayValue operator[](const char*){return {networks};}} doc;
struct Preferences {
 std::map<std::string,String> values;int count=0;
 void remove(const char* k){values.erase(k);}
 void putString(const char* k,String v){values[k]=v;}
 void putInt(const char*,int n){count=n;}
 int getInt(const char*,int){return count;}
 String getString(const char* k,const char* fallback){auto i=values.find(k);return i==values.end()?String(fallback):i->second;}
 void end(){}
} preferences;
struct Cred {String ssid,password;};
struct Settings {std::vector<Cred> wifiCredentials;bool enableWifi=true;} settings;
constexpr int MAX_WIFI_NETWORKS=5;
bool isRebootPending=false;unsigned long rebootTime=0,now=10000;
unsigned long millis(){return now;}
void show_popup(const char*){} void ui_tick(){} void lv_task_handler(){} void delay(int){}
struct ESPMock {int restarts=0;void restart(){++restarts;}} ESP;
struct WifiMock {void disconnect(bool,bool){}} WiFi;
void* wifiScanTaskHandle=nullptr;
void wifiScanTask(void*){}
int scans=0;
void xTaskCreate(void(*)(void*),const char*,int,void*,int,void**){++scans;}
#define pdMS_TO_TICKS(x) (x)
void vTaskDelay(int){}
'''
suffix = r'''
int main(){
 preferences.putString("unrelated","preserved");
 settings.wifiCredentials={{"test-old","test-value"}};
 preferences.putString("wifi_ssid_0","test-old");preferences.putString("wifi_pass_0","test-value");preferences.count=1;
 save();assert(preferences.count==0);assert(!preferences.values.count("wifi_ssid_0"));assert(!preferences.values.count("wifi_pass_0"));
 assert(isRebootPending&&scans==0&&ESP.restarts==0);
 now=rebootTime+2999;checkReboot();assert(ESP.restarts==0);
 now=rebootTime+3000;checkReboot();assert(ESP.restarts==1);
 reload();assert(settings.wifiCredentials.empty());assert(preferences.values.at("unrelated")=="preserved");
 isRebootPending=false;doc.present=false;save();assert(!isRebootPending);
 doc.present=true;doc.networks={{"test-new","test-new-value"}};save();reload();
 assert(settings.wifiCredentials.size()==1&&settings.wifiCredentials[0].ssid=="test-new");
 assert(settings.wifiCredentials[0].password=="test-new-value");
 puts("PASS: last-network removal, delayed reload, replacement and unrelated-state preservation");
}
'''
with tempfile.TemporaryDirectory(prefix='v1-wifi-removal-') as tmp:
    path = Path(tmp)
    (path/'test.cpp').write_text(prefix + 'void save(){\n' + save + '\n}\nvoid reload(){\n' + load + '\n}\n' + reboot + suffix)
    subprocess.run(['clang++', '-std=c++17', '-fsanitize=address,undefined', str(path/'test.cpp'), '-o', str(path/'test')], check=True)
    subprocess.run([str(path/'test')], check=True)
