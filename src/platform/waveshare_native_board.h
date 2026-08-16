#ifndef WAVESHARE_NATIVE_BOARD_H
#define WAVESHARE_NATIVE_BOARD_H

#include <stdint.h>

enum class WaveshareNativeTouchGesture : uint8_t
{
    None = 0,
    Up,
    Down,
    Left,
    Right,
    Click,
    DoubleClick,
    LongPress,
};

struct WaveshareNativeTouchSample
{
    bool touching;
    uint16_t x;
    uint16_t y;
    WaveshareNativeTouchGesture gesture;
};

bool waveshare_native_begin();
uint16_t *waveshare_native_framebuffer();
void waveshare_native_clear(uint16_t color);
void waveshare_native_present_full();
void waveshare_native_present_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void waveshare_native_present_set_min_frame_ms(uint16_t min_frame_ms);
bool waveshare_native_is_double_buffer_active();
bool waveshare_native_read_tilt(float *tilt_x, float *tilt_y);
bool waveshare_native_poll_touch(WaveshareNativeTouchSample *sample);
bool waveshare_native_poll_click(bool *clicked);

#endif
