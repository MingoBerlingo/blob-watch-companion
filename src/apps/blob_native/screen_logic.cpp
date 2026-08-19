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
        enum class ActiveScreen : uint8_t
        {
            Blob = 0,
            Timer,
        };

        ActiveScreen g_current_screen = ActiveScreen::Blob;
        ActiveScreen g_previous_screen = ActiveScreen::Blob;

        void capture_screen_state()
        {
            g_current_screen = timer_screen_active() ? ActiveScreen::Timer : ActiveScreen::Blob;
        }

        bool screen_state_changed()
        {
            return g_current_screen != g_previous_screen;
        }

        void commit_screen_state()
        {
            g_previous_screen = g_current_screen;
        }

        void reset_screen_state()
        {
            g_current_screen = ActiveScreen::Blob;
            commit_screen_state();
        }

        void cleanup_for_transition()
        {
            waveshare_native_clear(BG_COLOR);

            if (g_current_screen == ActiveScreen::Blob)
            {
                blob_screen_reset();
            }

            if (g_current_screen == ActiveScreen::Timer)
            {
                timer_renderer_reset();
            }
        }

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
        reset_screen_state();
        timer_renderer_reset();
        blob_screen_reset();
    }

    void blob_screen_manager_loop(uint32_t frame_start_us, uint32_t now_ms)
    {
        WaveshareNativeTouchSample touch_sample = {false, 0, 0, WaveshareNativeTouchGesture::None};
        waveshare_native_poll_touch(&touch_sample);
        timer_handle_touch(touch_sample, now_ms);
        timer_update_remaining(now_ms);

        if (waveshare_native_framebuffer() == NULL)
        {
            delay(50);
            return;
        }

        capture_screen_state();
        if (screen_state_changed())
        {
            cleanup_for_transition();
            // Push the cleared frame immediately so stale pixels do not persist
            // when the next frame is rendered with partial dirty updates.
            waveshare_native_present_full();
            commit_screen_state();
            cap_frame_rate(frame_start_us);
            return;
        }

        if (g_current_screen == ActiveScreen::Timer)
        {
            timer_renderer_draw_frame();
            cap_frame_rate(frame_start_us);
            return;
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
