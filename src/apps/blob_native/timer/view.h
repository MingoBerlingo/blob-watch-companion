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

    enum class TimerField : uint8_t
    {
        Minutes = 0,
        Seconds,
    };

    // Lifecycle
    void timer_update_remaining(uint32_t now_ms);
    void timer_enter();
    void timer_exit();

    // State transitions (driven by the LVGL timer page)
    void timer_select_minutes();
    void timer_select_seconds();
    void timer_start(uint32_t now_ms);
    void timer_hide_controls();
    void timer_toggle_controls();
    void timer_action(uint32_t now_ms);
    void timer_cancel();
    void timer_apply_value_delta(int delta);

    // Accessors
    TimerView timer_view();
    TimerField timer_active_field();
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
