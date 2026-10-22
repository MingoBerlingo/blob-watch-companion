#include "runtime.h"
#include <SDL.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Arduino.h"
#include "lvgl.h"
#include "apps/blob_native/timer/view.h"

namespace simulator {
namespace {
int passed = 0;
std::vector<std::string> failures;
int g_flush_calls = 0;
void (*g_original_flush)(lv_disp_drv_t *, const lv_area_t *, lv_color_t *) = nullptr;
void count_flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels)
{
    ++g_flush_calls;
    g_original_flush(driver, area, pixels);
}
void expect(bool condition, const std::string &message)
{
    if (condition) { ++passed; std::cout << "PASS " << message << '\n'; }
    else { failures.push_back(message); std::cerr << "FAIL " << message << '\n'; }
}
lv_obj_t *find(lv_obj_t *root, const lv_obj_class_t *type, const char *text = nullptr)
{
    if (lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN)) return nullptr;
    if (lv_obj_check_type(root, type) && (!text || std::string(lv_label_get_text(root)) == text)) return root;
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(root); ++i)
        if (auto *child = find(lv_obj_get_child(root, i), type, text)) return child;
    return nullptr;
}
lv_obj_t *widget(const lv_obj_class_t *type, const char *text = nullptr)
{
    auto *object = find(lv_scr_act(), type, text);
    if (!object) throw std::runtime_error(std::string("Missing visible widget: ") + (text ? text : type->base_class ? "class" : "root"));
    return object;
}
void release()
{
    auto sample = touch();
    sample.touching = false;
    sample.gesture = WaveshareNativeTouchGesture::None;
    set_touch(sample);
    step(100);
}
void swipe(WaveshareNativeTouchGesture direction)
{
    release();
    set_touch({true, 20, 120, direction});
    step(150); // Repeat the same controller report across many real page loops.
    set_touch({false, 20, 120, direction});
    step(100); // Registers can retain the gesture after lifting the finger.
    release();
}
void click(int x, int y)
{
    set_touch({true, static_cast<uint16_t>(x), static_cast<uint16_t>(y), WaveshareNativeTouchGesture::None});
    step(100);
    release();
}
void button(const char *text)
{
    auto *object = lv_obj_get_parent(widget(&lv_label_class, text));
    lv_area_t area;
    lv_obj_get_coords(object, &area);
    click((area.x1 + area.x2) / 2, (area.y1 + area.y2) / 2);
}
void drag_slider(lv_obj_t *slider, bool high)
{
    lv_area_t area;
    lv_obj_get_coords(slider, &area);
    const int x = (area.x1 + area.x2) / 2;
    const int start = (area.y1 + area.y2) / 2;
    const int end = high ? area.y1 - 10 : area.y2 + 10;
    for (int i = 0; i <= 10; ++i)
    {
        const int y = start + (end - start) * i / 10;
        set_touch({true, static_cast<uint16_t>(x), static_cast<uint16_t>(y), WaveshareNativeTouchGesture::None});
        step(40);
    }
    release();
}
void set_duration(int seconds, int minutes = 0)
{
    // Drive the same LVGL change callbacks used by the on-device rollers.
    int roller_index = 0;
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(lv_scr_act()); ++i)
    {
        auto *object = lv_obj_get_child(lv_scr_act(), i);
        if (lv_obj_check_type(object, &lv_roller_class))
        {
            lv_roller_set_selected(object, roller_index++ == 0 ? minutes : seconds, LV_ANIM_OFF);
            lv_event_send(object, LV_EVENT_VALUE_CHANGED, nullptr);
        }
    }
    step(100);
}
}
bool run_scenarios(const std::filesystem::path &output)
{
    using namespace blob_native;
    using Gesture = WaveshareNativeTouchGesture;
    auto capture = [&](const char *name) { screenshot(output / (std::string(name) + ".bmp")); };
    auto *blob_screen = lv_scr_act();
    capture("blob");
    expect(!timer_screen_active(), "Blob is the initial page");
    swipe(Gesture::Left);
    auto *timer_screen = lv_scr_act();
    expect(timer_screen != blob_screen && timer_view() == TimerView::TimerSetup, "One held swipe stops on Timer");
    expect(find(lv_scr_act(), &lv_roller_class) != nullptr, "Timer renders its real rollers");
    capture("timer-setup");
    swipe(Gesture::Left);
    auto *brightness_screen = lv_scr_act();
    expect(brightness_screen != timer_screen, "Next swipe opens Brightness");
    auto *slider = widget(&lv_slider_class);
    expect(lv_obj_get_width(slider) == 60 && lv_obj_get_height(slider) == 150, "Slider is vertical and 60 pixels wide");
    expect(lv_slider_get_value(slider) == 70, "Brightness starts at 70 percent");
    expect(lv_obj_get_child_cnt(brightness_screen) == 2 && find(brightness_screen, &lv_label_class, "brigthness"), "Brightness contains only its title and slider");
    capture("brightness");
    drag_slider(slider, false);
    expect(lv_slider_get_value(slider) == 5 && brightness() == 5, "Pointer drag reaches minimum and updates backlight request");
    expect(lv_scr_act() == brightness_screen, "Vertical dragging does not navigate");
    capture("brightness-low");
    drag_slider(slider, true);
    expect(lv_slider_get_value(slider) == 100 && brightness() == 100, "Pointer drag reaches maximum and updates backlight request");
    capture("brightness-high");
    swipe(Gesture::Right);
    expect(lv_scr_act() == timer_screen, "Reverse swipe stops on Timer");
    swipe(Gesture::Left);
    expect(lv_scr_act() == brightness_screen && lv_slider_get_value(slider) == 100, "Brightness survives leaving and returning");
    swipe(Gesture::Right);
    set_duration(0, 10);
    button("Start");
    expect(timer_running() && timer_minutes() == 10, "Reference flow starts a ten minute timer");
    capture("timer-reference-running");
    click(120, 164);
    expect(!timer_running() && timer_run_controls_visible(), "Reference pause shows the three icon actions");
    capture("timer-reference-paused");
    click(120, 164);
    advance(600100);
    step(100);
    expect(timer_view() == TimerView::TimerEnded, "Reference countdown opens its ended screen");
    capture("timer-reference-ended");
    click(82, 138);
    set_duration(5);
    expect(timer_minutes() == 0 && timer_seconds() == 5, "LVGL roller callbacks configure a five second timer");
    button("Start");
    expect(timer_running() && timer_view() == TimerView::TimerRun, "Pointer click starts timer");
    expect(lv_obj_get_height(widget(&lv_label_class, "00:05")) >= 30, "Running time uses the large type from the reference");
    capture("timer-running");
    step(500);
    expect(timer_remaining_ms() < 5000 && timer_remaining_ms() > 0, "Simulated clock advances the countdown");
    click(120, 60);
    expect(timer_running() && !timer_run_controls_visible(), "Background taps leave running timer unchanged");
    click(120, 164);
    expect(!timer_running() && timer_run_controls_visible(), "Orange pause button freezes timer and opens actions");
    const auto remaining = timer_remaining_ms();
    step(500);
    expect(timer_remaining_ms() == remaining, "Paused timer keeps remaining duration");
    expect(lv_obj_get_height(widget(&lv_label_class)) < 25, "Paused time uses smaller type above the actions");
    capture("timer-paused");
    click(82, 98);
    expect(lv_scr_act() == blob_screen && !timer_running(), "Back action opens Blob with timer still paused");
    step(500);
    expect(timer_remaining_ms() == remaining, "Timer remains frozen while viewing Blob");
    swipe(Gesture::Left);
    expect(lv_scr_act() == timer_screen && timer_run_controls_visible(), "Returning to paused timer restores actions");
    click(120, 164);
    expect(timer_running() && !timer_run_controls_visible(), "Green play button resumes and hides actions");
    advance(remaining + 100);
    step(100);
    expect(!timer_running() && timer_remaining_ms() == 0 && timer_view() == TimerView::TimerEnded, "Timer expiry opens the ended screen");
    expect(find(lv_scr_act(), &lv_label_class, "Timer ended") != nullptr, "Ended screen announces completion");
    expect(find(lv_scr_act(), &lv_label_class, "00:05") != nullptr, "Ended screen shows the original duration");
    capture("timer-expired");
    click(158, 138);
    expect(timer_running() && timer_remaining_ms() > 4500 && !timer_run_controls_visible(), "Orange repeat button restarts the original duration");
    click(120, 164);
    click(158, 98);
    expect(timer_view() == TimerView::TimerSetup && !timer_running(), "Red cancel button returns to timer setup");
    set_duration(1);
    button("Start");
    advance(1100);
    step(100);
    click(82, 138);
    expect(timer_view() == TimerView::TimerSetup, "Ended back button returns to duration selection");
    set_duration(0);
    button("Start");
    expect(!timer_running() && timer_view() == TimerView::TimerSetup, "Zero duration does not start a timer");
    swipe(Gesture::Right);
    expect(lv_scr_act() == blob_screen, "Reverse navigation returns to Blob");
    float x, y;
    set_tilt(2, -2);
    read_tilt(&x, &y);
    expect(x == 1 && y == -1, "Simulated tilt clamps to the hardware range");
    set_tilt(0, 0);
    const auto expected_color = static_cast<uint16_t>(0x781F);
    // Test the board boundary with the same byte-swapped RGB565 sent by LVGL.
    const uint16_t swapped = static_cast<uint16_t>((expected_color >> 8) | (expected_color << 8));
    flush(120, 120, 120, 120, &swapped);
    screenshot(output / "color-check.bmp");
    // Use SDL's real event queue and window scaling even in the dummy driver.
    auto *window = SDL_GetWindowFromID(window_id());
    if (!window) throw std::runtime_error("Missing simulator SDL window");
    SDL_SetWindowSize(window, 960, 720); // 120px letterbox on each side, scale 3x.
    SDL_Event event{};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 120; event.button.y = 0;
    SDL_PushEvent(&event);
    handle_events();
    expect(!touch().touching, "Mouse press outside the circular screen is rejected");
    event.button.x = 480; event.button.y = 360;
    SDL_PushEvent(&event);
    handle_events();
    expect(touch().touching && touch().x == 120 && touch().y == 120, "Resized letterboxed window maps mouse to display pixels");
    event.type = SDL_MOUSEMOTION;
    event.motion.x = 300; event.motion.y = 360;
    SDL_PushEvent(&event);
    handle_events();
    expect(touch().x == 60 && touch().y == 120, "Mouse drag uses the same scaled coordinates");
    event = {};
    event.type = SDL_MOUSEBUTTONUP;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 300; event.button.y = 360;
    SDL_PushEvent(&event);
    handle_events();
    expect(!touch().touching && touch().gesture == Gesture::Left, "Horizontal mouse release emits one swipe");
    handle_events();
    expect(touch().gesture == Gesture::None, "Mouse swipe clears on the next event poll");
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.x = 480; event.button.y = 360;
    SDL_PushEvent(&event);
    handle_events();
    event = {};
    event.type = SDL_WINDOWEVENT;
    event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    SDL_PushEvent(&event);
    handle_events();
    expect(!touch().touching, "Focus loss releases a held mouse touch");
    release();
    swipe(Gesture::Left);
    set_duration(5);
    lv_area_t start_area;
    lv_obj_get_coords(lv_obj_get_parent(widget(&lv_label_class, "Start")), &start_area);
    event = {};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 120 + (start_area.x1 + start_area.x2) * 3 / 2;
    event.button.y = (start_area.y1 + start_area.y2) * 3 / 2;
    SDL_PushEvent(&event);
    event.type = SDL_MOUSEBUTTONUP;
    SDL_PushEvent(&event);
    handle_events(); // Both transitions arrive before one poll.
    step(100);
    expect(timer_running(), "Batched SDL down and up activate the real Start button");
    click(120, 164); // Pause before checking idle rendering and the next short tap.
    step(100);
    auto *display_driver = lv_disp_get_default()->driver;
    g_original_flush = display_driver->flush_cb;
    g_flush_calls = 0;
    display_driver->flush_cb = count_flush;
    step(100);
    display_driver->flush_cb = g_original_flush;
    expect(g_flush_calls == 0, "Paused timer does not redraw unchanged controls");

    // A short press immediately after a poll is the worst case for sampling.
    lv_timer_reset(lv_indev_get_next(nullptr)->driver->read_timer);
    set_touch({true, 120, 164, Gesture::None});
    step(10);
    release();
    expect(timer_running() && !timer_run_controls_visible(), "Ten millisecond tap resumes the paused timer");
    std::ofstream report(output / "report.json");
    report << "{\"passed\":" << passed << ",\"failed\":" << failures.size() << ",\"failures\":[";
    for (size_t i = 0; i < failures.size(); ++i) report << (i ? "," : "") << '"' << failures[i] << '"';
    report << "]}\n";
    if (!report) throw std::runtime_error("Could not write test report");
    return failures.empty();
}
}
