#ifndef TIMER_RUNTIME_DRAW_H
#define TIMER_RUNTIME_DRAW_H

#include "lvgl.h"

namespace blob_native
{
    enum class TimerIcon : uint8_t { Pause = 1, Play, Back, Cancel, Repeat };

    lv_obj_t *timer_create_icon_button(lv_obj_t *parent, TimerIcon icon, uint16_t color,
                                       int16_t center_x, int16_t center_y);
    lv_obj_t *timer_create_progress_ring(lv_obj_t *parent);
    void timer_refresh_progress_ring(lv_obj_t *ring, uint32_t remaining, uint32_t total);
}

#endif
