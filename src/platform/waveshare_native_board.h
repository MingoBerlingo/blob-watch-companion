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
void waveshare_native_clear_display(uint16_t color);
void waveshare_native_flush_area(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint16_t *pixels);
bool waveshare_native_read_tilt(float *tilt_x, float *tilt_y);
bool waveshare_native_poll_touch(WaveshareNativeTouchSample *sample);

#endif
