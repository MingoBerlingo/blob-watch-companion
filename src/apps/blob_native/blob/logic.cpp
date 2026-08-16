#include "apps/blob_native/blob/view.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "apps/blob_native/blob/face.h"
#include "apps/blob_native/blob/overlay.h"
#include "apps/blob_native/blob/shape.h"
#include "platform/waveshare_native_board.h"

namespace blob_native
{

    namespace
    {
        struct BlobRuntimeState
        {
            float t;
            bool first;
            float blob_x;
            float blob_y;
            float vel_x;
            float vel_y;
            float prev_cx;
            float prev_cy;
            float prev_speed;
            int16_t prev_px[POINTS];
            int16_t prev_py[POINTS];
            uint16_t overlay_fps;
            uint16_t overlay_frame_count;
            uint32_t overlay_window_start_ms;
        };

        BlobRuntimeState g_blob_runtime = {
            0.0f,
            true,
            CENTER_X,
            CENTER_Y,
            0.0f,
            0.0f,
            CENTER_X,
            CENTER_Y,
            0.0f,
            {0},
            {0},
            0,
            0,
            0,
        };
    } // namespace

    void blob_screen_reset()
    {
        g_blob_runtime.t = 0.0f;
        g_blob_runtime.first = true;
        g_blob_runtime.blob_x = CENTER_X;
        g_blob_runtime.blob_y = CENTER_Y;
        g_blob_runtime.vel_x = 0.0f;
        g_blob_runtime.vel_y = 0.0f;
        g_blob_runtime.prev_cx = CENTER_X;
        g_blob_runtime.prev_cy = CENTER_Y;
        g_blob_runtime.prev_speed = 0.0f;
        memset(g_blob_runtime.prev_px, 0, sizeof(g_blob_runtime.prev_px));
        memset(g_blob_runtime.prev_py, 0, sizeof(g_blob_runtime.prev_py));
        g_blob_runtime.overlay_fps = 0;
        g_blob_runtime.overlay_frame_count = 0;
        g_blob_runtime.overlay_window_start_ms = 0;
    }

    bool blob_screen_prepare_frame(BlobScreenFrame *frame)
    {
        if (frame == NULL || waveshare_native_framebuffer() == NULL)
        {
            return false;
        }

        float cx = CENTER_X + cosf(g_blob_runtime.t * 0.7f) * 24.0f;
        float cy = CENTER_Y + sinf(g_blob_runtime.t * 0.9f) * 18.0f;
        float vx = cosf(g_blob_runtime.t * 1.2f) * 0.8f;
        float vy = sinf(g_blob_runtime.t * 1.1f) * 0.8f;

        if (BLOB_RAW_BLOB_USE_IMU)
        {
            float tilt_x = 0.0f;
            float tilt_y = 0.0f;
            if (waveshare_native_read_tilt(&tilt_x, &tilt_y))
            {
                const float target_x = CENTER_X + tilt_y * (CENTER_X - BLOB_RADIUS - 6.0f);
                const float target_y = CENTER_Y - tilt_x * (CENTER_Y - BLOB_RADIUS - 6.0f);

                g_blob_runtime.vel_x = g_blob_runtime.vel_x * 0.75f + (target_x - g_blob_runtime.blob_x) * 0.15f;
                g_blob_runtime.vel_y = g_blob_runtime.vel_y * 0.75f + (target_y - g_blob_runtime.blob_y) * 0.15f;
                g_blob_runtime.blob_x += g_blob_runtime.vel_x;
                g_blob_runtime.blob_y += g_blob_runtime.vel_y;

                cx = g_blob_runtime.blob_x;
                cy = g_blob_runtime.blob_y;
                vx = g_blob_runtime.vel_x;
                vy = g_blob_runtime.vel_y;
            }
        }

        const float speed = sqrtf(vx * vx + vy * vy);

        frame->phase_t = g_blob_runtime.t;
        frame->cx = cx;
        frame->cy = cy;
        frame->speed = speed;

        g_blob_state.phase_t = g_blob_runtime.t;
        compute_blob(cx, cy, vx, vy, frame->px, frame->py);

        frame->min_x = SCREEN_W - 1;
        frame->min_y = SCREEN_H - 1;
        frame->max_x = 0;
        frame->max_y = 0;
        update_bounds(frame->px, frame->py, &frame->min_x, &frame->min_y, &frame->max_x, &frame->max_y);
        update_glow_bounds(cx, cy, frame->px, frame->py, &frame->min_x, &frame->min_y, &frame->max_x, &frame->max_y);
        update_eyes_bounds(cx, cy, FACE_DIR_X, FACE_DIR_Y, speed, g_blob_runtime.t,
                           EyesAnimState::Idle, &frame->min_x, &frame->min_y, &frame->max_x, &frame->max_y);
        update_mouth_bounds(cx, cy, FACE_DIR_X, FACE_DIR_Y, speed, g_blob_runtime.t,
                            MouthAnimState::Neutral, &frame->min_x, &frame->min_y, &frame->max_x, &frame->max_y);

        if (!g_blob_runtime.first)
        {
            update_bounds(g_blob_runtime.prev_px, g_blob_runtime.prev_py, &frame->min_x, &frame->min_y, &frame->max_x, &frame->max_y);
            update_glow_bounds(g_blob_runtime.prev_cx, g_blob_runtime.prev_cy, g_blob_runtime.prev_px, g_blob_runtime.prev_py,
                               &frame->min_x, &frame->min_y, &frame->max_x, &frame->max_y);
            update_eyes_bounds(g_blob_runtime.prev_cx, g_blob_runtime.prev_cy, FACE_DIR_X, FACE_DIR_Y, g_blob_runtime.prev_speed,
                               g_blob_runtime.t - 0.08f, EyesAnimState::Idle,
                               &frame->min_x, &frame->min_y, &frame->max_x, &frame->max_y);
            update_mouth_bounds(g_blob_runtime.prev_cx, g_blob_runtime.prev_cy, FACE_DIR_X, FACE_DIR_Y, g_blob_runtime.prev_speed,
                                g_blob_runtime.t - 0.08f, MouthAnimState::Neutral,
                                &frame->min_x, &frame->min_y, &frame->max_x, &frame->max_y);
        }

        frame->min_x = clamp_i16(frame->min_x - DIRTY_MARGIN, 0, SCREEN_W - 1);
        frame->min_y = clamp_i16(frame->min_y - DIRTY_MARGIN, 0, SCREEN_H - 1);
        frame->max_x = clamp_i16(frame->max_x + DIRTY_MARGIN, 0, SCREEN_W - 1);
        frame->max_y = clamp_i16(frame->max_y + DIRTY_MARGIN, 0, SCREEN_H - 1);

        frame->overlay_redraw = false;
        frame->overlay_fps = g_blob_runtime.overlay_fps;
        if (BLOB_PERF_OVERLAY_ENABLED)
        {
            g_blob_runtime.overlay_frame_count++;
            const uint32_t now_ms = millis();
            if (g_blob_runtime.overlay_window_start_ms == 0)
            {
                g_blob_runtime.overlay_window_start_ms = now_ms;
                frame->overlay_redraw = true;
            }

            const uint32_t elapsed_ms = now_ms - g_blob_runtime.overlay_window_start_ms;
            if (elapsed_ms >= BLOB_PERF_OVERLAY_UPDATE_MS)
            {
                g_blob_runtime.overlay_fps = (uint16_t)((g_blob_runtime.overlay_frame_count * 1000u) / elapsed_ms);
                g_blob_runtime.overlay_frame_count = 0;
                g_blob_runtime.overlay_window_start_ms = now_ms;
                frame->overlay_redraw = true;
                frame->overlay_fps = g_blob_runtime.overlay_fps;
            }

            if (frame->overlay_redraw)
            {
                overlay_expand_dirty_bounds(&frame->min_x, &frame->min_y, &frame->max_x, &frame->max_y);
            }
        }

        return true;
    }

    void blob_screen_commit_frame(const BlobScreenFrame &frame)
    {
        memcpy(g_blob_runtime.prev_px, frame.px, sizeof(frame.px));
        memcpy(g_blob_runtime.prev_py, frame.py, sizeof(frame.py));
        g_blob_runtime.prev_cx = frame.cx;
        g_blob_runtime.prev_cy = frame.cy;
        g_blob_runtime.prev_speed = frame.speed;
        g_blob_runtime.first = false;
        g_blob_runtime.t += 0.08f;
    }

} // namespace blob_native
