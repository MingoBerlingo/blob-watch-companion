#ifndef TIMER_VIEW_H
#define TIMER_VIEW_H

#include <stdint.h>

#include "platform/waveshare_native_board.h"

namespace blob_native
{

    enum class TimerView : uint8_t
    {
        MainScreen = 0,
        TimerSetup,
        TimerRun,
    };

    void timer_handle_touch(const WaveshareNativeTouchSample &touch, uint32_t now_ms);
    void timer_update_remaining(uint32_t now_ms);

    TimerView timer_view();
    bool timer_screen_active();
    bool timer_run_controls_visible();
    int32_t timer_display_seconds_value();

    void timer_draw_setup_overlay();
    void timer_draw_run_ring();
    void timer_draw_run_time_overlay();
    void timer_draw_run_controls_overlay();
    void timer_run_center_bounds(int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);

    void timer_renderer_reset();
    void timer_renderer_draw_frame();

} // namespace blob_native

#endif
