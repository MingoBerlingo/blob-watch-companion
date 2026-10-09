#ifndef TIMER_VIEW_H
#define TIMER_VIEW_H

#include <stdint.h>

namespace blob_native
{

    enum class TimerView : uint8_t
    {
        MainScreen = 0,
        TimerSetup,
        TimerRun,
    };

    // Lifecycle
    void timer_update_remaining(uint32_t now_ms);
    void timer_enter();
    void timer_exit();

    // State transitions (driven by the LVGL timer page)
    void timer_set_minutes(int minutes);
    void timer_set_seconds(int seconds);
    void timer_start(uint32_t now_ms);
    void timer_hide_controls();
    void timer_toggle_controls();
    void timer_action(uint32_t now_ms);
    void timer_cancel();

    // Accessors
    TimerView timer_view();
    bool timer_screen_active();
    bool timer_running();
    bool timer_run_controls_visible();
    int timer_minutes();
    int timer_seconds();
    uint32_t timer_total_ms();
    uint32_t timer_remaining_ms();
    int32_t timer_display_seconds_value();
    int32_t timer_total_seconds();

} // namespace blob_native

#endif
