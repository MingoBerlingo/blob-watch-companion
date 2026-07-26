#include "apps/blob_native/blob_native_framebuffer.h"

#include <string.h>

#include "apps/blob_native/blob_native_blob.h"
#include "apps/blob_native/blob_native_face.h"
#include "platform/waveshare_native_board.h"

namespace blob_native
{

    int rect_width(const Rect &rect)
    {
        return rect.x1 - rect.x0 + 1;
    }

    int rect_height(const Rect &rect)
    {
        return rect.y1 - rect.y0 + 1;
    }

    Rect rect_union(const Rect &a, const Rect &b)
    {
        Rect merged;
        merged.x0 = (a.x0 < b.x0) ? a.x0 : b.x0;
        merged.y0 = (a.y0 < b.y0) ? a.y0 : b.y0;
        merged.x1 = (a.x1 > b.x1) ? a.x1 : b.x1;
        merged.y1 = (a.y1 > b.y1) ? a.y1 : b.y1;
        return merged;
    }

    Rect make_blob_rect(float cx, float cy, float eye_x, float eye_y, float motion_speed, float face_phase,
                        const int16_t *px, const int16_t *py)
    {
        Rect rect = {SCREEN_W - 1, SCREEN_H - 1, 0, 0};
        update_bounds(px, py, &rect.x0, &rect.y0, &rect.x1, &rect.y1);
        update_glow_bounds(cx, cy, px, py, &rect.x0, &rect.y0, &rect.x1, &rect.y1);
        update_eyes_bounds(cx, cy, eye_x, eye_y, motion_speed, face_phase, &rect.x0, &rect.y0, &rect.x1, &rect.y1);
        update_mouth_bounds(cx, cy, eye_x, eye_y, motion_speed, face_phase, &rect.x0, &rect.y0, &rect.x1, &rect.y1);

        rect.x0 = clamp_i16(rect.x0 - DIRTY_MARGIN, 0, SCREEN_W - 1);
        rect.y0 = clamp_i16(rect.y0 - DIRTY_MARGIN, 0, SCREEN_H - 1);
        rect.x1 = clamp_i16(rect.x1 + DIRTY_MARGIN, 0, SCREEN_W - 1);
        rect.y1 = clamp_i16(rect.y1 + DIRTY_MARGIN, 0, SCREEN_H - 1);
        return rect;
    }

    void save_region_pixels(const Rect &rect)
    {
        uint16_t *framebuffer = waveshare_native_framebuffer();
        const int width = rect_width(rect);
        const int height = rect_height(rect);

        for (int row = 0; row < height; row++)
        {
            memcpy(&g_blob_state.saved_region[row * width], &framebuffer[(rect.y0 + row) * SCREEN_W + rect.x0], width * sizeof(uint16_t));
        }
    }

    void restore_region_pixels(const Rect &rect)
    {
        uint16_t *framebuffer = waveshare_native_framebuffer();
        const int width = rect_width(rect);
        const int height = rect_height(rect);

        for (int row = 0; row < height; row++)
        {
            memcpy(&framebuffer[(rect.y0 + row) * SCREEN_W + rect.x0], &g_blob_state.saved_region[row * width], width * sizeof(uint16_t));
        }
    }

} // namespace blob_native