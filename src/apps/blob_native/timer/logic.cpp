#include "apps/blob_native/timer/internal.h"

#include <Arduino.h>

namespace blob_native
{

    namespace timer_internal
    {
        TimerUiState g_timer_ui = {
            TimerView::MainScreen,
            TimerField::Minutes,
            5,
            0,
            5u * 60u * 1000u,
            0,
            5u * 60u * 1000u,
            false,
            false,
            0,
            0,
            0,
            0,
            false,
            false,
            false,
        };

        int16_t g_ring_x[kTimerRingSegments] = {0};
        int16_t g_ring_y[kTimerRingSegments] = {0};
        bool g_ring_points_ready = false;

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

        void timer_apply_value_delta(int delta)
        {
            if (g_timer_ui.active_field == TimerField::Minutes)
            {
                g_timer_ui.minutes_set = clamp_i32(g_timer_ui.minutes_set + delta, kTimerMinMinutes, kTimerMaxMinutes);
            }
            else
            {
                g_timer_ui.seconds_set = clamp_i32(g_timer_ui.seconds_set + delta, kTimerMinSeconds, kTimerMaxSeconds);
            }
            timer_sync_total_ms();
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

        int timer_active_segments()
        {
            if (g_timer_ui.total_ms == 0)
            {
                return 0;
            }

            const float progress = constrain((float)timer_display_seconds() / ((float)g_timer_ui.total_ms / 1000.0f), 0.0f, 1.0f);
            return clamp_i32((int)(progress * (float)kTimerRingSegments + 0.5f), 0, kTimerRingSegments);
        }

        bool timer_has_runtime_view()
        {
            return g_timer_ui.running ||
                   g_timer_ui.controls_visible ||
                   g_timer_ui.view == TimerView::TimerRun ||
                   (g_timer_ui.total_ms > 0 && g_timer_ui.remaining_ms != g_timer_ui.total_ms);
        }
    } // namespace timer_internal

    void timer_handle_touch(const WaveshareNativeTouchSample &touch, uint32_t now_ms)
    {
        using namespace timer_internal;

        const bool click_gesture = (touch.gesture == WaveshareNativeTouchGesture::Click ||
                                    touch.gesture == WaveshareNativeTouchGesture::DoubleClick);
        const bool new_click = click_gesture && !g_timer_ui.click_latched;
        if (click_gesture)
        {
            g_timer_ui.click_latched = true;
        }
        else if (!touch.touching || touch.gesture == WaveshareNativeTouchGesture::None)
        {
            g_timer_ui.click_latched = false;
        }

        if (touch.touching)
        {
            g_timer_ui.last_touch_x = (uint16_t)clamp_i16((int16_t)touch.x, 0, SCREEN_W - 1);
            g_timer_ui.last_touch_y = (uint16_t)clamp_i16((int16_t)touch.y, 0, SCREEN_H - 1);
            g_timer_ui.has_last_touch = true;
        }

        if (g_timer_ui.view == TimerView::MainScreen)
        {
            if (touch.gesture == WaveshareNativeTouchGesture::Left)
            {
                g_timer_ui.view = timer_has_runtime_view() ? TimerView::TimerRun : TimerView::TimerSetup;
                g_timer_ui.dragging = false;
                g_timer_ui.drag_accum_px = 0;
            }
            return;
        }

        if (g_timer_ui.view == TimerView::TimerSetup && touch.gesture == WaveshareNativeTouchGesture::Right)
        {
            g_timer_ui.view = TimerView::MainScreen;
            g_timer_ui.dragging = false;
            g_timer_ui.drag_accum_px = 0;
            return;
        }

        if (g_timer_ui.view == TimerView::TimerRun)
        {
            if (touch.gesture == WaveshareNativeTouchGesture::Right)
            {
                g_timer_ui.view = TimerView::MainScreen;
                g_timer_ui.dragging = false;
                g_timer_ui.drag_accum_px = 0;
                g_timer_ui.controls_visible = false;
                return;
            }

            if (new_click)
            {
                if (g_timer_ui.controls_visible && g_timer_ui.has_last_touch)
                {
                    const uint16_t tx = g_timer_ui.last_touch_x;
                    const uint16_t ty = g_timer_ui.last_touch_y;

                    if (ui::button_contains(kPauseButton, tx, ty))
                    {
                        g_timer_ui.running = false;
                        g_timer_ui.controls_visible = false;
                        return;
                    }

                    if (ui::button_contains(kEraseButton, tx, ty))
                    {
                        g_timer_ui.running = false;
                        g_timer_ui.remaining_ms = g_timer_ui.total_ms;
                        g_timer_ui.controls_visible = false;
                        g_timer_ui.view = TimerView::TimerSetup;
                        return;
                    }
                }

                g_timer_ui.controls_visible = !g_timer_ui.controls_visible;
            }

            return;
        }

        if (new_click)
        {
            if (g_timer_ui.has_last_touch)
            {
                const uint16_t tx = g_timer_ui.last_touch_x;
                const uint16_t ty = g_timer_ui.last_touch_y;

                if (ui::button_contains(kMinutesButton, tx, ty))
                {
                    g_timer_ui.active_field = TimerField::Minutes;
                }
                else if (ui::button_contains(kSecondsButton, tx, ty))
                {
                    g_timer_ui.active_field = TimerField::Seconds;
                }
                else if (ui::button_contains(kStartButton, tx, ty))
                {
                    timer_start(now_ms);
                    g_timer_ui.dragging = false;
                    g_timer_ui.drag_accum_px = 0;
                    return;
                }
            }
        }

        if (!touch.touching)
        {
            g_timer_ui.dragging = false;
            g_timer_ui.drag_accum_px = 0;
            return;
        }

        const int16_t y = clamp_i16((int16_t)touch.y, 0, SCREEN_H - 1);
        if (!g_timer_ui.dragging)
        {
            g_timer_ui.dragging = true;
            g_timer_ui.drag_last_y = y;
            g_timer_ui.drag_accum_px = 0;
            return;
        }

        const int dy = (int)g_timer_ui.drag_last_y - (int)y;
        g_timer_ui.drag_last_y = y;
        g_timer_ui.drag_accum_px += (int16_t)dy;

        const int steps = g_timer_ui.drag_accum_px / kTimerAdjustPxPerStep;
        if (steps != 0)
        {
            timer_apply_value_delta(steps);
            g_timer_ui.drag_accum_px -= (int16_t)(steps * kTimerAdjustPxPerStep);
        }
    }

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

    TimerView timer_view()
    {
        return timer_internal::g_timer_ui.view;
    }

    bool timer_screen_active()
    {
        return timer_internal::g_timer_ui.view != TimerView::MainScreen;
    }

    bool timer_run_controls_visible()
    {
        return timer_internal::g_timer_ui.controls_visible;
    }

    int32_t timer_display_seconds_value()
    {
        return timer_internal::timer_display_seconds();
    }

} // namespace blob_native
