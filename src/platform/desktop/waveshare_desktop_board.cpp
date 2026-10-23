// Compiled only by simulator/CMakeLists.txt; the firmware keeps its native adapter.
#ifdef BLOB_DESKTOP_SIMULATOR
#include "platform/waveshare_native_board.h"
#include "runtime.h"
#include <cstring>

bool waveshare_native_begin() { return true; }
void waveshare_native_set_brightness(uint8_t percent) { simulator::set_brightness(percent); }
void waveshare_native_clear_display(uint16_t color) { simulator::clear(color); }
void waveshare_native_flush_area(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint16_t *pixels)
{
    simulator::flush(x0, y0, x1, y1, pixels);
}
bool waveshare_native_read_motion(WaveshareNativeMotionSample *sample)
{
    if (!sample) return false;
    simulator::read_motion(sample);
    return true;
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
namespace { uint8_t g_settings_data[256]{}; size_t g_settings_size=0; }
bool waveshare_native_load_settings(void *data,size_t size) {
    if(!data || size!=g_settings_size) return false;
    memcpy(data,g_settings_data,size); return true;
}
bool waveshare_native_save_settings(const void *data,size_t size) {
    if(!data || size>sizeof(g_settings_data)) return false;
    memcpy(g_settings_data,data,size);g_settings_size=size;return true;
}
bool waveshare_native_read_acceleration(float *x,float *y,float *z){
    if(!x||!y||!z) return false;
    simulator::read_acceleration(x,y,z);return true;
}
namespace {uint8_t g_step_data[128]{};size_t g_step_size=0;}
bool waveshare_native_load_step_data(void *data,size_t size){
    if(!data || size!=g_step_size) return false;
    memcpy(data,g_step_data,size);return true;
}
bool waveshare_native_save_step_data(const void *data,size_t size){
    if(!data || size>sizeof(g_step_data)) return false;
    memcpy(g_step_data,data,size);g_step_size=size;return true;
}
#endif
