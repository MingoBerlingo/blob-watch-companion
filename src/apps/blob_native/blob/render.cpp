#include "apps/blob_native/blob/view.h"

#include "apps/blob_native/blob/face.h"
#include "apps/blob_native/blob/overlay.h"
#include "apps/blob_native/blob/shape.h"
#include "platform/waveshare_native_board.h"

namespace blob_native
{

    void blob_screen_render_frame(const BlobScreenFrame &frame)
    {
        uint16_t *fb = waveshare_native_framebuffer();
        if (fb == NULL)
        {
            return;
        }

        const uint16_t black_swapped = (uint16_t)((BLACK << 8) | (BLACK >> 8));
        for (int y = frame.min_y; y <= frame.max_y; y++)
        {
            uint16_t *row = &fb[y * SCREEN_W + frame.min_x];
            for (int x = frame.min_x; x <= frame.max_x; x++)
            {
                *row++ = black_swapped;
            }
        }

        draw_blob_glow(frame.cx, frame.cy, frame.px, frame.py, OUTLINE_COLOR);
        if (BLOB_FILL_ENABLED)
        {
            draw_blob_fill(frame.px, frame.py, BLOB_COLOR);
        }
        draw_blob_outline(frame.px, frame.py, OUTLINE_COLOR);
        draw_blob_eyes(frame.cx, frame.cy, FACE_DIR_X, FACE_DIR_Y, frame.speed, frame.phase_t,
                       EyesAnimState::Idle, EYE_COLOR);
        draw_blob_mouth(frame.cx, frame.cy, FACE_DIR_X, FACE_DIR_Y, frame.speed, frame.phase_t,
                        MouthAnimState::Neutral, MOUTH_COLOR);

        if (BLOB_PERF_OVERLAY_ENABLED && frame.overlay_redraw)
        {
            overlay_draw(fb, frame.overlay_fps, frame.max_x - frame.min_x + 1, frame.max_y - frame.min_y + 1);
        }

        if (BLOB_FORCE_FULL_PRESENT)
        {
            waveshare_native_present_full();
        }
        else
        {
            waveshare_native_present_window(frame.min_x, frame.min_y, frame.max_x, frame.max_y);
        }
    }

} // namespace blob_native
