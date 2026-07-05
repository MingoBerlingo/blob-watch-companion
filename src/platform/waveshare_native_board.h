#ifndef WAVESHARE_NATIVE_BOARD_H
#define WAVESHARE_NATIVE_BOARD_H

#include <stdint.h>

bool waveshare_native_begin();
uint16_t* waveshare_native_framebuffer();
void waveshare_native_clear(uint16_t color);
void waveshare_native_present_full();
void waveshare_native_present_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
bool waveshare_native_read_tilt(float* tilt_x, float* tilt_y);

#endif
