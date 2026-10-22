"""Exercise real timer state transitions without LVGL or hardware."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    temp = Path(directory)
    (temp / 'Arduino.h').write_text('#include <stdint.h>\n')
    (temp / 'timer.cpp').write_text('''#include <cassert>
#include "apps/blob_native/timer/view.h"
using namespace blob_native;
int main() {
    timer_enter();
    timer_set_minutes(0);
    timer_set_seconds(5);
    timer_start(100);
    timer_action(1350);
    assert(!timer_running() && timer_run_controls_visible());
    assert(timer_remaining_ms() == 3750);
    timer_update_remaining(2000);
    assert(timer_remaining_ms() == 3750);
    timer_exit();
    timer_enter();
    assert(timer_view() == TimerView::TimerRun && timer_run_controls_visible());
    timer_action(3000);
    assert(timer_running() && !timer_run_controls_visible());
    timer_update_remaining(6750);
    assert(timer_remaining_ms() == 0 && !timer_running() && timer_view() == TimerView::TimerEnded);
    timer_exit();
    timer_enter();
    assert(timer_view() == TimerView::TimerEnded);
    timer_action(7000);
    assert(timer_running() && timer_remaining_ms() == 5000 && !timer_run_controls_visible());
    timer_cancel();
    assert(timer_view() == TimerView::TimerSetup && !timer_run_controls_visible());
    timer_start(8000);
    timer_action(8000);
    timer_exit();
    timer_enter();
    assert(timer_view() == TimerView::TimerRun && timer_run_controls_visible());
    assert(timer_remaining_ms() == 5000); // Full-duration pause survives exit/reentry.
    timer_cancel();
    timer_start(0xFFFFFF00u);
    timer_action(0x00000100u);
    assert(!timer_running() && timer_remaining_ms() == 4488);
    timer_cancel();
    timer_start(9000);
    timer_action(14000);
    assert(!timer_running() && timer_remaining_ms() == 0 && timer_view() == TimerView::TimerEnded);
    timer_cancel();
    timer_start(15000);
    timer_exit();
    timer_update_remaining(21000);
    assert(!timer_screen_active()); // Expiry must not steal focus from Blob/Brightness.
    timer_enter();
    assert(timer_view() == TimerView::TimerEnded);
    timer_action(22000);
    assert(timer_running() && timer_remaining_ms() == 5000);
    timer_cancel();
    timer_set_seconds(0);
    timer_start(100);
    assert(!timer_running() && timer_view() == TimerView::TimerSetup);
}
''')
    executable = temp / 'timer'
    subprocess.run(['c++', '-std=c++11', '-I', str(temp), '-I', str(ROOT / 'src'),
                    str(temp / 'timer.cpp'), str(ROOT / 'src/apps/blob_native/timer/logic.cpp'),
                    '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('Timer logic regression tests passed')
