#pragma once
#include <cstdint>
#include <filesystem>
#include "platform/waveshare_native_board.h"

namespace simulator {
constexpr int width = 240;
constexpr int height = 240;
void init(bool headless);
void shutdown();
void advance(uint32_t milliseconds);
void step(uint32_t milliseconds = 10);
void set_touch(const WaveshareNativeTouchSample &sample);
WaveshareNativeTouchSample touch();
void set_brightness(uint8_t percent);
uint8_t brightness();
void set_tilt(float x, float y);
void read_tilt(float *x, float *y);
void flush(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint16_t *pixels);
void clear(uint16_t color);
void present();
void screenshot(const std::filesystem::path &path);
bool handle_events();
bool fast_clock();
uint32_t window_id();
bool run_scenarios(const std::filesystem::path &output);
}
