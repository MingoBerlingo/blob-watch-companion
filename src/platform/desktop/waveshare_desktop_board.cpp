// Compiled only by simulator/CMakeLists.txt; the firmware keeps its native adapter.
#ifdef BLOB_DESKTOP_SIMULATOR
#include "platform/waveshare_native_board.h"
#include "runtime.h"

bool waveshare_native_begin() { return true; }
void waveshare_native_set_brightness(uint8_t percent) { simulator::set_brightness(percent); }
void waveshare_native_clear_display(uint16_t color) { simulator::clear(color); }
void waveshare_native_flush_area(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint16_t *pixels)
{
    simulator::flush(x0, y0, x1, y1, pixels);
}
bool waveshare_native_read_tilt(float *x, float *y)
{
    if (!x || !y) return false;
    simulator::read_tilt(x, y);
    return true;
}
bool waveshare_native_poll_touch(WaveshareNativeTouchSample *sample)
{
    if (!sample) return false;
    *sample = simulator::touch();
    return true;
}
#endif
