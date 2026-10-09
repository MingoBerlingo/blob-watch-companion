#include "apps/blob_native/timer/internal.h"

#include <Arduino.h>

namespace blob_native
{

    namespace timer_internal
    {
        TimerUiState g_timer_ui = {
            TimerView::MainScreen,
            5,
            0,
            5u * 60u * 1000u,
            0,
            5u * 60u * 1000u,
            false,
            false,
        };

        int clamp_i32(int v, int lo, int hi)
        {
            if (v < lo)
                return lo;
            if (v > hi)
                return hi;
            return v;
        }

        void timer_sync_total_ms()
        {
            const uint32_t total_sec = (uint32_t)g_timer_ui.minutes_set * 60u + (uint32_t)g_timer_ui.seconds_set;
            g_timer_ui.total_ms = total_sec * 1000u;
            g_timer_ui.remaining_ms = g_timer_ui.total_ms;
        }

        void timer_start(uint32_t now_ms)
        {
            timer_sync_total_ms();
            if (g_timer_ui.total_ms == 0)
            {
                g_timer_ui.running = false;
                g_timer_ui.remaining_ms = 0;
                return;
            }

            g_timer_ui.end_ms = now_ms + g_timer_ui.total_ms;
            g_timer_ui.remaining_ms = g_timer_ui.total_ms;
            g_timer_ui.running = true;
            g_timer_ui.controls_visible = false;
            g_timer_ui.view = TimerView::TimerRun;
        }

        int32_t timer_display_seconds()
        {
            if (g_timer_ui.total_ms == 0)
            {
                return 0;
            }

            if (g_timer_ui.running || g_timer_ui.view == TimerView::TimerRun)
            {
                return (int32_t)((g_timer_ui.remaining_ms + 999u) / 1000u);
            }

            return (int32_t)(g_timer_ui.total_ms / 1000u);
        }

        bool timer_has_runtime_view()
        {
            return g_timer_ui.running ||
                   g_timer_ui.controls_visible ||
                   g_timer_ui.view == TimerView::TimerRun ||
                   (g_timer_ui.total_ms > 0 && g_timer_ui.remaining_ms != g_timer_ui.total_ms);
        }
    } // namespace timer_internal

    // ---- Public API ----

    void timer_update_remaining(uint32_t now_ms)
    {
        using namespace timer_internal;

        if (!g_timer_ui.running)
        {
            return;
        }

        const int32_t delta = (int32_t)(g_timer_ui.end_ms - now_ms);
        if (delta <= 0)
        {
            g_timer_ui.running = false;
            g_timer_ui.remaining_ms = 0;
            g_timer_ui.controls_visible = true;
            return;
        }

        g_timer_ui.remaining_ms = (uint32_t)delta;
    }

    void timer_enter()
    {
        using namespace timer_internal;
        g_timer_ui.view = timer_has_runtime_view() ? TimerView::TimerRun : TimerView::TimerSetup;
    }

    void timer_exit()
    {
        using namespace timer_internal;
        g_timer_ui.view = TimerView::MainScreen;
        g_timer_ui.controls_visible = false;
    }

    void timer_set_minutes(int minutes)
    {
        using namespace timer_internal;
        g_timer_ui.minutes_set = clamp_i32(minutes, kTimerMinMinutes, kTimerMaxMinutes);
        timer_sync_total_ms();
    }

    void timer_set_seconds(int seconds)
    {
        using namespace timer_internal;
        g_timer_ui.seconds_set = clamp_i32(seconds, kTimerMinSeconds, kTimerMaxSeconds);
        timer_sync_total_ms();
    }

    void timer_start(uint32_t now_ms)
    {
        timer_internal::timer_start(now_ms);
    }

    void timer_hide_controls()
    {
        timer_internal::g_timer_ui.controls_visible = false;
    }

    void timer_toggle_controls()
    {
        timer_internal::g_timer_ui.controls_visible = !timer_internal::g_timer_ui.controls_visible;
    }

    void timer_action(uint32_t now_ms)
    {
        using namespace timer_internal;
        if (g_timer_ui.running)
        {
            g_timer_ui.running = false;
        }
        else if (g_timer_ui.remaining_ms > 0)
        {
            g_timer_ui.end_ms = now_ms + g_timer_ui.remaining_ms;
            g_timer_ui.running = true;
        }
    }

    void timer_cancel()
    {
        using namespace timer_internal;
        g_timer_ui.running = false;
        g_timer_ui.remaining_ms = g_timer_ui.total_ms;
        g_timer_ui.controls_visible = false;
        g_timer_ui.view = TimerView::TimerSetup;
    }

    TimerView timer_view()
    {
        return timer_internal::g_timer_ui.view;
    }

    bool timer_screen_active()
    {
        return timer_internal::g_timer_ui.view != TimerView::MainScreen;
    }

    bool timer_running()
    {
        return timer_internal::g_timer_ui.running;
    }

    bool timer_run_controls_visible()
    {
        return timer_internal::g_timer_ui.controls_visible;
    }

    int timer_minutes()
    {
        return timer_internal::g_timer_ui.minutes_set;
    }

    int timer_seconds()
    {
        return timer_internal::g_timer_ui.seconds_set;
    }

    uint32_t timer_total_ms()
    {
        return timer_internal::g_timer_ui.total_ms;
    }

    uint32_t timer_remaining_ms()
    {
        return timer_internal::g_timer_ui.remaining_ms;
    }

    int32_t timer_display_seconds_value()
    {
        return timer_internal::timer_display_seconds();
    }

    int32_t timer_total_seconds()
    {
        return (int32_t)(timer_internal::g_timer_ui.total_ms / 1000u);
    }

} // namespace blob_native
