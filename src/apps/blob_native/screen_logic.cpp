#include "apps/blob_native/screen_logic.h"

#include <Arduino.h>

#include "apps/blob_native/blob/view.h"
#include "apps/blob_native/shared_state.h"
#include "apps/blob_native/timer/view.h"
#include "platform/waveshare_native_board.h"

namespace blob_native
{

    namespace
    {
        TimerView g_prev_timer_view = TimerView::MainScreen;
        bool g_prev_timer_controls_visible = false;
        int32_t g_prev_timer_display_seconds = -1;

        void cap_frame_rate(uint32_t frame_start_us)
        {
            const uint32_t frame_elapsed_us = micros() - frame_start_us;
            constexpr uint32_t FRAME_TARGET_US = 16667;
            if (frame_elapsed_us < FRAME_TARGET_US)
            {
                delayMicroseconds(FRAME_TARGET_US - frame_elapsed_us);
            }
        }
    } // namespace

    void blob_screen_manager_reset()
    {
        g_prev_timer_view = TimerView::MainScreen;
        g_prev_timer_controls_visible = false;
        g_prev_timer_display_seconds = -1;
        blob_screen_reset();
    }

    void blob_screen_manager_loop(uint32_t frame_start_us, uint32_t now_ms)
    {
        WaveshareNativeTouchSample touch_sample = {false, 0, 0, WaveshareNativeTouchGesture::None};
        waveshare_native_poll_touch(&touch_sample);
        timer_handle_touch(touch_sample, now_ms);
        timer_update_remaining(now_ms);

        const TimerView current_timer_view = timer_view();
        const bool timer_controls_visible = timer_run_controls_visible();
        const int32_t timer_display_seconds = timer_display_seconds_value();

        if (waveshare_native_framebuffer() == NULL)
        {
            delay(50);
            return;
        }

        if (current_timer_view != TimerView::MainScreen)
        {
            const bool view_changed = (g_prev_timer_view != current_timer_view);

            if (view_changed)
            {
                waveshare_native_clear(BG_COLOR);

                if (current_timer_view == TimerView::TimerSetup)
                {
                    timer_draw_setup_overlay();
                }
                else
                {
                    timer_draw_run_ring();
                    if (timer_controls_visible)
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
            else if (current_timer_view == TimerView::TimerSetup)
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
                timer_run_center_bounds(&cx0, &cy0, &cx1, &cy1);

                if (timer_display_seconds != g_prev_timer_display_seconds)
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

                    if (timer_controls_visible)
                    {
                        timer_draw_run_controls_overlay();
                    }
                    else
                    {
                        timer_draw_run_time_overlay();
                    }

                    waveshare_native_present_full();
                }
                else if (timer_controls_visible != g_prev_timer_controls_visible)
                {
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

                    if (timer_controls_visible)
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

            g_prev_timer_view = current_timer_view;
            g_prev_timer_controls_visible = timer_controls_visible;
            g_prev_timer_display_seconds = timer_display_seconds;
            cap_frame_rate(frame_start_us);
            return;
        }

        if (g_prev_timer_view != TimerView::MainScreen)
        {
            waveshare_native_clear(BG_COLOR);
            blob_screen_reset();
            g_prev_timer_controls_visible = false;
            g_prev_timer_display_seconds = -1;
            g_prev_timer_view = TimerView::MainScreen;
        }

        BlobScreenFrame frame;
        if (!blob_screen_prepare_frame(&frame))
        {
            delay(16);
            return;
        }

        blob_screen_render_frame(frame);
        blob_screen_commit_frame(frame);
        cap_frame_rate(frame_start_us);
    }

} // namespace blob_native
