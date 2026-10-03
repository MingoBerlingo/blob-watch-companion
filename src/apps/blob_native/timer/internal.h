#ifndef TIMER_INTERNAL_H
#define TIMER_INTERNAL_H

#include <stdint.h>

#include "apps/blob_native/timer/view.h"

namespace blob_native
{

    namespace timer_internal
    {
        constexpr int kTimerMinMinutes = 0;
        constexpr int kTimerMaxMinutes = 99;
        constexpr int kTimerMinSeconds = 0;
        constexpr int kTimerMaxSeconds = 99;

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
            bool controls_visible;
        };

        extern TimerUiState g_timer_ui;

        int clamp_i32(int v, int lo, int hi);
        void timer_sync_total_ms();
        void timer_apply_value_delta(int delta);
        void timer_start(uint32_t now_ms);
        int32_t timer_display_seconds();
        bool timer_has_runtime_view();

    } // namespace timer_internal

} // namespace blob_native

#endif
