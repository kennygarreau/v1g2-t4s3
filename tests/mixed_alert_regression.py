"""Exercise the actual BLE notification callback with host-side collaborators.

The decoder mock supplies the existing display-status mute-bit contract.
This tests callback state ownership, not BLE hardware or the entire decoder.
"""
from pathlib import Path
import subprocess
import tempfile
import sys

source = Path(sys.argv[1] if len(sys.argv) > 1 else "src/ble.cpp").read_text()
start = source.index("static void notifyDisplayCallbackv2(")
end = source.index("\nvoid displayReader(", start)
callback = source[start:end]
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
struct NimBLERemoteCharacteristic {};
struct Characteristic {
  void setValue(uint8_t*, size_t) {}
  void notify() {}
};
Characteristic* pAlertNotifyChar = nullptr;
struct { bool proxyBLE = false; int lowSpeedThreshold = 40; } settings;
bool notifySubscribed = false, needsMode = false, newDataAvailable = false;
bool alertPresent = true, photoAlertPresent = false, muted = false;
int bleNotifyMutex = 0, currentSpeed = 0;
int pdMS_TO_TICKS(int v) { return v; }
bool xSemaphoreTake(int, int) { return true; }
void xSemaphoreGive(int) {}
void start_clear_inactive_bands_timer() {}
struct PacketDecoder {
  std::vector<uint8_t> packet;
  PacketDecoder(std::vector<uint8_t> p) : packet(p) {}
  void decode_v2(int, int) {
    if (packet[3] == 0x31) muted = (packet[8] & 0x10) != 0;
  }
};
'''
suffix = r'''
int main() {
  uint8_t alert[14] = {0xAA}; alert[3] = 0x43;
  alertPresent = true;
  photoAlertPresent = true;
  alert[5] = 0x12; // First of two alerts: do not switch to the idle screen.
  for (int i = 0; i < 100; ++i) {
    notifyDisplayCallbackv2(nullptr, alert, sizeof(alert), true);
    assert(alertPresent);
    assert(photoAlertPresent); // Preserve state until complete-table decoding.
    alert[5] = 0x22;
    notifyDisplayCallbackv2(nullptr, alert, sizeof(alert), true);
    assert(alertPresent);
    alert[5] = 0x12;
  }
  alert[5] = 0;
  notifyDisplayCallbackv2(nullptr, alert, sizeof(alert), true);
  assert(!alertPresent && !photoAlertPresent); // Genuine empty table clears.
  puts("PASS: 100 mixed-alert cycles and genuine clearing");
}
'''
with tempfile.TemporaryDirectory(prefix="v1-mute-test-") as tmp:
    path = Path(tmp)
    (path / "test.cpp").write_text(prefix + callback + suffix)
    subprocess.run(["clang++", "-std=c++17", "-fsanitize=address,undefined",
                    str(path / "test.cpp"), "-o", str(path / "test")], check=True)
    result = subprocess.run([str(path / "test")], capture_output=True, text=True)
    if result.returncode:
        print("FAIL:", result.stderr.splitlines()[0] if result.stderr else result.returncode)
        sys.exit(1)
    print(result.stdout.strip())
