#include "runtime.h"
#include "lvgl.h"
#include <SDL.h>
#include <iostream>

namespace simulator {
namespace {
uint64_t pixels = 0;
uint32_t frames = 0;
void monitor(lv_disp_drv_t *, uint32_t, uint32_t count) {
    pixels += count;
    ++frames;
}
}
void benchmark_blob() {
    step(1000); // Exclude startup and the one-time contour lookup construction.
    auto *driver = lv_disp_get_default()->driver;
    const auto original = driver->monitor_cb;
    driver->monitor_cb = monitor;
    pixels = frames = 0;
    const auto start = SDL_GetPerformanceCounter();
    step(10000);
    const double milliseconds = 1000.0 * (SDL_GetPerformanceCounter() - start) / SDL_GetPerformanceFrequency();
    driver->monitor_cb = original;
    std::cout << "Blob: " << frames << " rendered frames / 10 simulated seconds; "
              << (frames ? pixels / frames : 0) << " transferred pixels/frame; "
              << (frames ? milliseconds / frames : 0) << " desktop ms/frame.\n"
              << "Desktop timing excludes the RP2040 CPU and physical SPI transfers.\n";
}
}
