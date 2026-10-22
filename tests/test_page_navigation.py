"""Host regression test for repeated touch-controller swipe reports."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    temp = Path(directory)
    headers = {
        'apps/blob_native/blob/physics_settings.h': 'namespace blob_native { void physics_settings_reset(){} bool physics_settings_load(){return false;} }',
        'apps/blob_native/pages/settings/settings_page.h': 'void settings_page_init(){} void settings_page_show(){}',
        'Arduino.h': '#include <stdint.h>\nuint32_t millis(){return 0;}\nvoid delay(int){}\n',
        'apps/blob_native/lv_port/lv_port.h': 'void lv_port_handler(){} void lv_port_block_touch_until_release(){}',
        'apps/blob_native/pages/blob/blob_page.h': 'void blob_page_init(){} void blob_page_show(){} void blob_page_hide(){}',
        'apps/blob_native/pages/brightness/brightness_page.h': 'void brightness_page_init(){} void brightness_page_show(){} void brightness_page_hide(){}',
        'apps/blob_native/pages/timer/timer_page.h': 'void timer_page_init(){} void timer_page_show(){} void timer_page_hide(){} void timer_page_tick(uint32_t){}',
        'apps/blob_native/timer/view.h': 'void timer_enter(){} void timer_exit(){} void timer_update_remaining(uint32_t){}',
    }
    for name, content in headers.items():
        path = temp / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
    source = temp / 'navigation.cpp'
    source.write_text('''#include <cassert>
#include "apps/blob_native/pages/page_manager.cpp"
bool waveshare_native_poll_touch(WaveshareNativeTouchSample*) { return true; }
int main() {
    using namespace blob_native;
    using Gesture = WaveshareNativeTouchGesture;
    page_manager_init();
    navigate({true, 0, 0, Gesture::Left});
    assert(g_page == Page::Timer);
    navigate({true, 0, 0, Gesture::Left});
    assert(g_page == Page::Timer); // A held gesture must not skip the timer.
    navigate({false, 0, 0, Gesture::Left});
    assert(g_page == Page::Timer); // The controller may retain it after release.
    navigate({false, 0, 0, Gesture::None});
    navigate({true, 0, 0, Gesture::Left});
    assert(g_page == Page::Brightness);
    navigate({false, 0, 0, Gesture::None});
    navigate({true, 0, 0, Gesture::Right});
    navigate({true, 0, 0, Gesture::Right});
    assert(g_page == Page::Timer);
    navigate({false, 0, 0, Gesture::Right});
    navigate({true, 0, 0, Gesture::None});
    navigate({true, 0, 0, Gesture::Right});
    assert(g_page == Page::Blob); // A fresh touch rearms even after a stale gesture.
}
''')
    executable = temp / 'navigation'
    subprocess.run(['c++', '-std=c++11', '-I', str(temp), '-I', str(ROOT / 'src'), str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('Page navigation regression test passed')
