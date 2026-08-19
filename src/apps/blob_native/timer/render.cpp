#include "apps/blob_native/timer/internal.h"

#include <math.h>
#include <stdio.h>

#include "apps/blob_native/ui/draw.h"

namespace blob_native
{

    namespace
    {
        // Fixed slot for MM:SS (always 5 chars) in timer run view.
        constexpr UWORD kTimerTextX = 78;
        constexpr UWORD kTimerTextY = 114;
    }

    namespace timer_internal
    {
        TimerView g_prev_view = TimerView::MainScreen;
        bool g_prev_controls_visible = false;
        int32_t g_prev_display_seconds = -1;

        void run_overlay_bounds(bool controls_visible, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
        {
            if (min_x == NULL || min_y == NULL || max_x == NULL || max_y == NULL)
            {
                return;
            }

            // Tight center box for time text updates.
            if (!controls_visible)
            {
                *min_x = 62;
                *min_y = 92;
                *max_x = 178;
                *max_y = 160;
                return;
            }

            // Larger bounds for control buttons to avoid stale icon pixels.
            *min_x = 38;
            *min_y = 46;
            *max_x = 202;
            *max_y = 198;
        }

        void ensure_ring_points()
        {
            if (g_ring_points_ready)
            {
                return;
            }

            for (int i = 0; i < kTimerRingSegments; i++)
            {
                const float a = -HALF_PI + (2.0f * PI * i) / (float)kTimerRingSegments;
                const float ca = cosf(a);
                const float sa = sinf(a);
                g_ring_x[i] = (int16_t)(CENTER_X + ca * (float)kTimerRingOuterRadius);
                g_ring_y[i] = (int16_t)(CENTER_Y + sa * (float)kTimerRingOuterRadius);
            }

            g_ring_points_ready = true;
        }

        void draw_ring_segment_line(int segment, uint16_t color)
        {
            if (segment < 0 || segment >= kTimerRingSegments)
            {
                return;
            }

            const int16_t ox = g_ring_x[segment];
            const int16_t oy = g_ring_y[segment];
            const float dx = (float)ox - CENTER_X;
            const float dy = (float)oy - CENTER_Y;
            const float length = sqrtf(dx * dx + dy * dy);
            if (length <= 0.001f)
            {
                return;
            }

            const float scale = (float)kTimerRingInnerRadius / length;
            const int16_t x0 = (int16_t)(CENTER_X + dx * scale);
            const int16_t y0 = (int16_t)(CENTER_Y + dy * scale);
            const int16_t x1 = ox;
            const int16_t y1 = oy;

            const float tx = -dy / length;
            const float ty = dx / length;

            for (int offset = -kRingStrokeHalfWidth; offset <= kRingStrokeHalfWidth; offset++)
            {
                const int16_t sx0 = (int16_t)(x0 + tx * offset);
                const int16_t sy0 = (int16_t)(y0 + ty * offset);
                const int16_t sx1 = (int16_t)(x1 + tx * offset);
                const int16_t sy1 = (int16_t)(y1 + ty * offset);
                draw_line(sx0, sy0, sx1, sy1, color);
            }
        }

        void draw_full_ring(int active_segments)
        {
            for (int i = 0; i < kTimerRingSegments; i++)
            {
                draw_ring_segment_line(i, (i < active_segments) ? kRingActiveColor : kRingTrackColor);
            }
        }

        void draw_pause_icon(int x0, int y0, int x1, int y1, uint16_t color)
        {
            const int mid_y0 = y0 + 10;
            const int mid_y1 = y1 - 10;
            const int bar0 = x0 + 22;
            const int bar1 = x0 + 30;
            const int bar2 = x1 - 30;
            const int bar3 = x1 - 22;
            for (int x = bar0; x <= bar1; x++)
            {
                draw_line(x, mid_y0, x, mid_y1, color);
            }
            for (int x = bar2; x <= bar3; x++)
            {
                draw_line(x, mid_y0, x, mid_y1, color);
            }
        }

        void draw_x_icon(int x0, int y0, int x1, int y1, uint16_t color)
        {
            draw_line(x0 + 20, y0 + 12, x1 - 20, y1 - 12, color);
            draw_line(x0 + 20, y1 - 12, x1 - 20, y0 + 12, color);
            draw_line(x0 + 21, y0 + 12, x1 - 19, y1 - 12, color);
            draw_line(x0 + 21, y1 - 12, x1 - 19, y0 + 12, color);
        }

        void draw_play_icon(int x0, int y0, int x1, int y1, uint16_t color)
        {
            const int left = x0 + 24;
            const int right = x1 - 22;
            const int top = y0 + 16;
            const int bottom = y1 - 16;
            const int mid = (top + bottom) / 2;
            draw_line(left, top, right, mid, color);
            draw_line(left, bottom, right, mid, color);
            draw_line(left + 1, top, right + 1, mid, color);
            draw_line(left + 1, bottom, right + 1, mid, color);
            draw_line(left + 2, top, right + 2, mid, color);
            draw_line(left + 2, bottom, right + 2, mid, color);
        }

        void draw_back_icon(int x0, int y0, int x1, int y1, uint16_t color)
        {
            const int left = x0 + 22;
            const int right = x1 - 22;
            const int mid = (y0 + y1) / 2;
            const int top = y0 + 18;
            const int bottom = y1 - 18;
            draw_line(right, top, left, mid, color);
            draw_line(right, bottom, left, mid, color);
            draw_line(right - 1, top, left - 1, mid, color);
            draw_line(right - 1, bottom, left - 1, mid, color);
        }

        void draw_run_controls()
        {
            ui::draw_button(kBackButton);
            ui::draw_button(kCancelButton);

            ui::CircleButton action_button = kActionButton;
            if (!g_timer_ui.running && g_timer_ui.remaining_ms > 0)
            {
                action_button.fill_color = 0x57EA;
                action_button.border_color = 0x57EA;
            }
            ui::draw_button(action_button);

            draw_back_icon(kBackButton.cx - kBackButton.radius, kBackButton.cy - kBackButton.radius,
                           kBackButton.cx + kBackButton.radius, kBackButton.cy + kBackButton.radius, BLACK);

            if (g_timer_ui.running)
            {
                draw_pause_icon(action_button.cx - action_button.radius, action_button.cy - action_button.radius,
                                action_button.cx + action_button.radius, action_button.cy + action_button.radius, BLACK);
            }
            else
            {
                draw_play_icon(action_button.cx - action_button.radius, action_button.cy - action_button.radius,
                               action_button.cx + action_button.radius, action_button.cy + action_button.radius, BLACK);
            }

            draw_x_icon(kCancelButton.cx - kCancelButton.radius, kCancelButton.cy - kCancelButton.radius,
                        kCancelButton.cx + kCancelButton.radius, kCancelButton.cy + kCancelButton.radius, BLACK);
        }
    } // namespace timer_internal

    void timer_draw_setup_overlay()
    {
        using namespace timer_internal;

        const uint16_t active_border = CYAN;
        const uint16_t idle_border = kUiBorderColor;

        ui::RectButton minutes_button = kMinutesButton;
        ui::RectButton seconds_button = kSecondsButton;
        minutes_button.border_color = (g_timer_ui.active_field == TimerField::Minutes) ? active_border : idle_border;
        seconds_button.border_color = (g_timer_ui.active_field == TimerField::Seconds) ? active_border : idle_border;
        ui::draw_button(minutes_button);
        ui::draw_button(seconds_button);

        char mins[8] = {0};
        char secs[8] = {0};
        snprintf(mins, sizeof(mins), "%02d", g_timer_ui.minutes_set);
        snprintf(secs, sizeof(secs), "%02d", g_timer_ui.seconds_set);

        Paint_DrawString_EN(kMinutesButton.x0 + 16, kMinutesButton.y0 + 30, mins, &Font24, WHITE, BLACK);
        Paint_DrawString_EN(kSecondsButton.x0 + 16, kSecondsButton.y0 + 30, secs, &Font24, WHITE, BLACK);

        ui::draw_button(kStartButton);
        Paint_DrawString_EN(kStartButton.x0 + 36, kStartButton.y0 + 12, "START", &Font16, BLACK, CYAN);
    }

    void timer_draw_run_ring()
    {
        using namespace timer_internal;

        ensure_ring_points();
        draw_full_ring(timer_active_segments());
    }

    void timer_draw_run_time_overlay()
    {
        using namespace timer_internal;

        char timer_text[12] = {0};
        const int32_t display_seconds = timer_display_seconds();
        const int32_t minutes = display_seconds / 60;
        const int32_t seconds = display_seconds % 60;
        snprintf(timer_text, sizeof(timer_text), "%02ld:%02ld", (long)minutes, (long)seconds);
        Paint_DrawString_EN(kTimerTextX, kTimerTextY, timer_text, &Font24, WHITE, BLACK);
    }

    void timer_draw_run_controls_overlay()
    {
        timer_internal::draw_run_controls();
    }

    void timer_run_center_bounds(int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
    {
        timer_internal::run_overlay_bounds(false, min_x, min_y, max_x, max_y);
    }

    void timer_renderer_reset()
    {
        timer_internal::g_prev_view = TimerView::MainScreen;
        timer_internal::g_prev_controls_visible = false;
        timer_internal::g_prev_display_seconds = -1;
    }

    void timer_renderer_draw_frame()
    {
        const TimerView current_view = timer_view();
        if (current_view == TimerView::MainScreen)
        {
            return;
        }

        const bool controls_visible = timer_run_controls_visible();
        const int32_t display_seconds = timer_display_seconds_value();
        const bool view_changed = (timer_internal::g_prev_view != current_view);

        if (view_changed)
        {
            waveshare_native_clear(BG_COLOR);

            if (current_view == TimerView::TimerSetup)
            {
                timer_draw_setup_overlay();
            }
            else
            {
                timer_draw_run_ring();
                if (controls_visible)
                {
                    timer_draw_run_controls_overlay();
                }
                else
                {
                    timer_draw_run_time_overlay();
                }
            }

            waveshare_native_present_full();
        }
        else if (current_view == TimerView::TimerSetup)
        {
            waveshare_native_clear(BG_COLOR);
            timer_draw_setup_overlay();
            waveshare_native_present_full();
        }
        else
        {
            int16_t cx0 = 0;
            int16_t cy0 = 0;
            int16_t cx1 = 0;
            int16_t cy1 = 0;
            timer_internal::run_overlay_bounds(controls_visible, &cx0, &cy0, &cx1, &cy1);

            if (display_seconds != timer_internal::g_prev_display_seconds)
            {
                timer_draw_run_ring();

                const uint16_t black_swapped = (uint16_t)((BLACK << 8) | (BLACK >> 8));
                uint16_t *fb = waveshare_native_framebuffer();
                for (int y = cy0; y <= cy1; y++)
                {
                    uint16_t *row = &fb[y * SCREEN_W + cx0];
                    for (int x = cx0; x <= cx1; x++)
                    {
                        *row++ = black_swapped;
                    }
                }

                if (controls_visible)
                {
                    timer_draw_run_controls_overlay();
                }
                else
                {
                    timer_draw_run_time_overlay();
                }

                waveshare_native_present_full();
            }
            else if (controls_visible != timer_internal::g_prev_controls_visible)
            {
                timer_internal::run_overlay_bounds(true, &cx0, &cy0, &cx1, &cy1);
                const uint16_t black_swapped = (uint16_t)((BLACK << 8) | (BLACK >> 8));
                uint16_t *fb = waveshare_native_framebuffer();
                for (int y = cy0; y <= cy1; y++)
                {
                    uint16_t *row = &fb[y * SCREEN_W + cx0];
                    for (int x = cx0; x <= cx1; x++)
                    {
                        *row++ = black_swapped;
                    }
                }

                if (controls_visible)
                {
                    timer_draw_run_controls_overlay();
                }
                else
                {
                    timer_draw_run_time_overlay();
                }

                waveshare_native_present_window(cx0, cy0, cx1, cy1);
            }
        }

        timer_internal::g_prev_view = current_view;
        timer_internal::g_prev_controls_visible = controls_visible;
        timer_internal::g_prev_display_seconds = display_seconds;
    }

} // namespace blob_native
