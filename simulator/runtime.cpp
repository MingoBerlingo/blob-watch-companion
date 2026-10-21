#include "runtime.h"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include "Arduino.h"
#include "lvgl.h"
#include "apps/blob_native/pages/page_manager.h"

namespace {
uint32_t clock_ms = 1;
WaveshareNativeTouchSample input = {false, 0, 0, WaveshareNativeTouchGesture::None};
uint8_t backlight = 70;
float tilt_x = 0, tilt_y = 0;
std::array<uint32_t, simulator::width * simulator::height> framebuffer{};
SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
SDL_Texture *texture = nullptr;
bool accelerated = false;
bool mouse_down = false;
int press_x = 0, press_y = 0;

void check(bool ok, const char *operation)
{
    if (!ok) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}
uint32_t rgb565(uint16_t value)
{
    const uint32_t r = (value >> 11) & 31;
    const uint32_t g = (value >> 5) & 63;
    const uint32_t b = value & 31;
    return 0xFF000000u | (((r << 3) | (r >> 2)) << 16) |
           (((g << 2) | (g >> 4)) << 8) | ((b << 3) | (b >> 2));
}
bool inside(int x, int y)
{
    const float dx = x - 119.5f, dy = y - 119.5f;
    return x >= 0 && x < simulator::width && y >= 0 && y < simulator::height &&
           dx * dx + dy * dy <= 120.0f * 120.0f;
}
std::array<uint32_t, simulator::width * simulator::height> masked()
{
    auto result = framebuffer;
    for (int y = 0; y < simulator::height; ++y)
        for (int x = 0; x < simulator::width; ++x)
            if (!inside(x, y)) result[y * simulator::width + x] = 0xFF181818;
    return result;
}
void coordinates(int screen_x, int screen_y, int &x, int &y)
{
    // Match SDL_RenderCopy's centered square viewport, including letterboxing.
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    const int side = std::max(1, std::min(w, h));
    x = static_cast<int>(std::floor((screen_x - (w - side) / 2.0) * simulator::width / side));
    y = static_cast<int>(std::floor((screen_y - (h - side) / 2.0) * simulator::height / side));
}
void point(int x, int y)
{
    input.x = static_cast<uint16_t>(std::clamp(x, 0, simulator::width - 1));
    input.y = static_cast<uint16_t>(std::clamp(y, 0, simulator::height - 1));
}
}

extern "C" uint32_t millis(void) { return clock_ms; }
extern "C" void delay(uint32_t ms) { clock_ms += ms; }

namespace simulator {
void init(bool headless)
{
    if (headless) SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
    check(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL_Init");
    clear(0);
    window = SDL_CreateWindow("Blob Watch — arrows: pages | W/X/A/D: tilt | Space: 10x | S: screenshot",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | (headless ? SDL_WINDOW_HIDDEN : 0));
    check(window != nullptr, "SDL_CreateWindow");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    check(renderer != nullptr, "SDL_CreateRenderer");
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    check(texture != nullptr, "SDL_CreateTexture");
}
void shutdown()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    texture = nullptr;
    renderer = nullptr;
    window = nullptr;
    SDL_Quit();
}
void advance(uint32_t ms) { clock_ms += ms; }
void step(uint32_t ms)
{
    // page_manager_loop advances the simulated clock by its firmware delay(5).
    const uint32_t start = millis();
    while (static_cast<uint32_t>(millis() - start) < ms) blob_native::page_manager_loop();
}
void set_touch(const WaveshareNativeTouchSample &sample) { input = sample; }
WaveshareNativeTouchSample touch() { return input; }
void set_brightness(uint8_t percent) { backlight = std::clamp<int>(percent, 0, 100); }
uint8_t brightness() { return backlight; }
void set_tilt(float x, float y)
{
    tilt_x = std::clamp(x, -1.0f, 1.0f);
    tilt_y = std::clamp(y, -1.0f, 1.0f);
}
void read_tilt(float *x, float *y) { *x = tilt_x; *y = tilt_y; }
void flush(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint16_t *pixels)
{
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
        {
            const uint16_t value = *pixels++;
            // lv_conf.h enables LV_COLOR_16_SWAP for the real GC9A01 panel.
            const uint16_t native = static_cast<uint16_t>((value >> 8) | (value << 8));
            if (x >= 0 && x < width && y >= 0 && y < height) framebuffer[y * width + x] = rgb565(native);
        }
}
void clear(uint16_t color) { framebuffer.fill(rgb565(color)); }
void present()
{
    if (!renderer) return;
    const auto pixels = masked();
    check(SDL_UpdateTexture(texture, nullptr, pixels.data(), width * sizeof(uint32_t)) == 0, "SDL_UpdateTexture");
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    const int side = std::min(w, h);
    SDL_Rect destination = {(w - side) / 2, (h - side) / 2, side, side};
    SDL_SetRenderDrawColor(renderer, 24, 24, 24, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, &destination);
    SDL_RenderPresent(renderer);
}
void screenshot(const std::filesystem::path &path)
{
    auto pixels = masked();
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(), width, height, 32,
        width * sizeof(uint32_t), SDL_PIXELFORMAT_ARGB8888);
    check(surface != nullptr, "SDL_CreateRGBSurfaceWithFormatFrom");
    const int result = SDL_SaveBMP(surface, path.string().c_str());
    SDL_FreeSurface(surface);
    check(result == 0, "SDL_SaveBMP");
}
bool handle_events()
{
    input.gesture = WaveshareNativeTouchGesture::None;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT) return false;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        {
            mouse_down = false;
            input.touching = false;
            input.gesture = WaveshareNativeTouchGesture::None;
        }
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            coordinates(event.button.x, event.button.y, press_x, press_y);
            mouse_down = inside(press_x, press_y);
            input.touching = mouse_down;
            point(press_x, press_y);
        }
        if (event.type == SDL_MOUSEMOTION && mouse_down)
        {
            int x, y;
            coordinates(event.motion.x, event.motion.y, x, y);
            point(x, y);
        }
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
        {
            int x, y;
            coordinates(event.button.x, event.button.y, x, y);
            point(x, y);
            const int dx = x - press_x, dy = y - press_y;
            if (mouse_down && std::abs(dx) >= 40 && std::abs(dx) > std::abs(dy) * 1.3f)
                input.gesture = dx < 0 ? WaveshareNativeTouchGesture::Left : WaveshareNativeTouchGesture::Right;
            input.touching = false;
            mouse_down = false;
        }
        // Deliver every pointer transition before a later queued event overwrites it.
        // Calling LVGL's input timer callback does not advance simulated time.
        if ((event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) ||
            (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) ||
            (event.type == SDL_MOUSEMOTION && mouse_down) ||
            (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST))
        {
            for (auto *device = lv_indev_get_next(nullptr); device; device = lv_indev_get_next(device))
                if (lv_indev_get_type(device) == LV_INDEV_TYPE_POINTER)
                    lv_indev_read_timer_cb(device->driver->read_timer);
        }
        if (event.type == SDL_KEYDOWN && !event.key.repeat)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE: return false;
            case SDLK_LEFT: input.touching = false; input.gesture = WaveshareNativeTouchGesture::Left; break;
            case SDLK_RIGHT: input.touching = false; input.gesture = WaveshareNativeTouchGesture::Right; break;
            case SDLK_w: set_tilt(tilt_x + 0.15f, tilt_y); break;
            case SDLK_s: screenshot("screenshot.bmp"); break;
            case SDLK_x: set_tilt(tilt_x - 0.15f, tilt_y); break;
            case SDLK_a: set_tilt(tilt_x, tilt_y - 0.15f); break;
            case SDLK_d: set_tilt(tilt_x, tilt_y + 0.15f); break;
            case SDLK_r: set_tilt(0, 0); break;
            case SDLK_SPACE: accelerated = !accelerated; break;
            default: break;
            }
        }
        if (event.type == SDL_KEYUP && (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT))
            input.touching = false;
    }
    return true;
}
bool fast_clock() { return accelerated; }
uint32_t window_id() { return SDL_GetWindowID(window); }
}
