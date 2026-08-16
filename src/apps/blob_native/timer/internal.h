#ifndef TIMER_INTERNAL_H
#define TIMER_INTERNAL_H

#include <stdint.h>

#include "apps/blob_native/shared_state.h"
#include "apps/blob_native/timer/view.h"
#include "apps/blob_native/ui/button.h"

namespace blob_native
{

    namespace timer_internal
    {
        enum class TimerField : uint8_t
        {
            Minutes = 0,
            Seconds,
        };

        constexpr int kTimerMinMinutes = 0;
        constexpr int kTimerMaxMinutes = 180;
        constexpr int kTimerMinSeconds = 0;
        constexpr int kTimerMaxSeconds = 59;
        constexpr int kTimerAdjustPxPerStep = 10;

        constexpr int kTimerRingOuterRadius = (SCREEN_W / 2) - 2;
        constexpr int kTimerRingInnerRadius = kTimerRingOuterRadius - 8;
        constexpr int kTimerRingSegments = 160;

        constexpr uint16_t kRingActiveColor = CYAN;
        constexpr uint16_t kRingTrackColor = BLACK;
        constexpr int kRingStrokeHalfWidth = 1;
        constexpr uint16_t kUiBorderColor = 0x39E7;

        constexpr ui::RectButton kMinutesButton = {42, 58, 112, 146, BLACK, kUiBorderColor, false};
        constexpr ui::RectButton kSecondsButton = {128, 58, 198, 146, BLACK, kUiBorderColor, false};
        constexpr ui::RectButton kStartButton = {50, 170, 190, 214, CYAN, CYAN, true};

        constexpr ui::CircleButton kPauseButton = {SCREEN_W / 2, 78, 28, 0x52AA, 0x52AA, true};
        constexpr ui::CircleButton kEraseButton = {SCREEN_W / 2, 158, 34, 0xF98C, 0xF98C, true};

        struct TimerUiState
        {
            TimerView view;
            TimerField active_field;
            int minutes_set;
            int seconds_set;
            uint32_t total_ms;
            uint32_t end_ms;
            uint32_t remaining_ms;
            bool running;
            bool dragging;
            int16_t drag_last_y;
            int16_t drag_accum_px;
            uint16_t last_touch_x;
            uint16_t last_touch_y;
            bool has_last_touch;
            bool controls_visible;
            bool click_latched;
        };

        extern TimerUiState g_timer_ui;
        extern int16_t g_ring_x[kTimerRingSegments];
        extern int16_t g_ring_y[kTimerRingSegments];
        extern bool g_ring_points_ready;

        int clamp_i32(int v, int lo, int hi);
        bool point_in_rect(uint16_t x, uint16_t y, int x0, int y0, int x1, int y1);
        void timer_sync_total_ms();
        void timer_apply_value_delta(int delta);
        void timer_start(uint32_t now_ms);
        int32_t timer_display_seconds();
        void ensure_ring_points();
        int timer_active_segments();
        bool timer_has_runtime_view();

    } // namespace timer_internal

} // namespace blob_native

#endif
