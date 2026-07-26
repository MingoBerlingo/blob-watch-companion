#include "apps/blob_native/blob_native_app.h"

#include <Arduino.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "apps/blob_native/blob_native_blob.h"
#include "apps/blob_native/blob_native_face.h"
#include "apps/blob_native/blob_native_framebuffer.h"
#include "apps/blob_native/blob_native_state.h"
#include "platform/waveshare_native_board.h"

using namespace blob_native;

void blob_native_app_setup()
{
    Serial.begin(115200);
    if (!waveshare_native_begin())
    {
        Serial.println("Native board init failed");
        return;
    }

    waveshare_native_clear(BG_COLOR);
    waveshare_native_present_full();

    // Pre-allocate background backup buffer used by save/restore rendering.
    g_blob_state.saved_region = (uint16_t *)malloc(BACKUP_PIXELS * sizeof(uint16_t));
    if (g_blob_state.saved_region != NULL)
    {
        g_blob_state.use_saved_region = true;
    }
    else
    {
        Serial.println("Blob backup buffer alloc failed; using geometry erase fallback");
    }

    // Initialize previous-frame geometry at screen center.
    for (int i = 0; i < POINTS; i++)
    {
        g_blob_state.px_old[i] = (int16_t)CENTER_X;
        g_blob_state.py_old[i] = (int16_t)CENTER_Y;
    }
}

void blob_native_app_loop()
{
    if (waveshare_native_framebuffer() == NULL)
    {
        delay(50);
        return;
    }

    float tilt_x = 0.0f;
    float tilt_y = 0.0f;
    waveshare_native_read_tilt(&tilt_x, &tilt_y);

    // Snapshot previous state before advancing simulation.
    const float prev_blob_x = g_blob_state.blob_x;
    const float prev_blob_y = g_blob_state.blob_y;
    const float prev_face_phase = g_blob_state.face_phase_prev;
    const float prev_speed = sqrtf(g_blob_state.vel_x * g_blob_state.vel_x + g_blob_state.vel_y * g_blob_state.vel_y);

    // Target position comes from board tilt.
    const float target_x = CENTER_X + tilt_y * (CENTER_X - BLOB_RADIUS - 6.0f);
    const float target_y = CENTER_Y - tilt_x * (CENTER_Y - BLOB_RADIUS - 6.0f);

    // Critically damped-ish spring motion for smooth blob movement.
    g_blob_state.vel_x = g_blob_state.vel_x * 0.75f + (target_x - g_blob_state.blob_x) * 0.15f;
    g_blob_state.vel_y = g_blob_state.vel_y * 0.75f + (target_y - g_blob_state.blob_y) * 0.15f;

    g_blob_state.blob_x += g_blob_state.vel_x;
    g_blob_state.blob_y += g_blob_state.vel_y;

    const float speed_now = sqrtf(g_blob_state.vel_x * g_blob_state.vel_x + g_blob_state.vel_y * g_blob_state.vel_y);

    int16_t px_new[POINTS];
    int16_t py_new[POINTS];
    compute_blob(g_blob_state.blob_x, g_blob_state.blob_y, g_blob_state.vel_x, g_blob_state.vel_y, px_new, py_new);

    // Face orientation is fixed intentionally.
    const float face_dir_x = FACE_DIR_X;
    const float face_dir_y = FACE_DIR_Y;

    const Rect current_rect = make_blob_rect(g_blob_state.blob_x, g_blob_state.blob_y, face_dir_x, face_dir_y,
                                             speed_now, g_blob_state.phase_t, px_new, py_new);

    int16_t min_x = SCREEN_W - 1;
    int16_t min_y = SCREEN_H - 1;
    int16_t max_x = 0;
    int16_t max_y = 0;

    if (g_blob_state.use_saved_region)
    {
        // Fast path: restore old background, save new background, then draw current frame.
        if (g_blob_state.has_saved_region)
        {
            restore_region_pixels(g_blob_state.saved_rect);
        }

        save_region_pixels(current_rect);

        Rect present_rect = current_rect;
        if (g_blob_state.has_saved_region)
        {
            present_rect = rect_union(g_blob_state.saved_rect, current_rect);
        }

        draw_blob_glow(g_blob_state.blob_x, g_blob_state.blob_y, px_new, py_new, OUTLINE_COLOR);
        if (BLOB_FILL_ENABLED)
        {
            draw_blob_fill(px_new, py_new, BLOB_COLOR);
        }
        draw_blob_outline(px_new, py_new, OUTLINE_COLOR);
        draw_blob_eyes(g_blob_state.blob_x, g_blob_state.blob_y, face_dir_x, face_dir_y,
                       speed_now, g_blob_state.phase_t, EYE_COLOR);
        draw_blob_mouth(g_blob_state.blob_x, g_blob_state.blob_y, face_dir_x, face_dir_y,
                        speed_now, g_blob_state.phase_t, MOUTH_COLOR);

        waveshare_native_present_window(present_rect.x0, present_rect.y0, present_rect.x1, present_rect.y1);

        g_blob_state.saved_rect = current_rect;
        g_blob_state.has_saved_region = true;
    }
    else
    {
        // Fallback path: explicit dirty rect and erase by overdraw.
        update_bounds(px_new, py_new, &min_x, &min_y, &max_x, &max_y);
        update_glow_bounds(g_blob_state.blob_x, g_blob_state.blob_y, px_new, py_new, &min_x, &min_y, &max_x, &max_y);
        update_eyes_bounds(g_blob_state.blob_x, g_blob_state.blob_y, face_dir_x, face_dir_y,
                           speed_now, g_blob_state.phase_t, &min_x, &min_y, &max_x, &max_y);
        update_mouth_bounds(g_blob_state.blob_x, g_blob_state.blob_y, face_dir_x, face_dir_y,
                            speed_now, g_blob_state.phase_t, &min_x, &min_y, &max_x, &max_y);

        if (!g_blob_state.first_frame)
        {
            update_bounds(g_blob_state.px_old, g_blob_state.py_old, &min_x, &min_y, &max_x, &max_y);
            update_glow_bounds(prev_blob_x, prev_blob_y, g_blob_state.px_old, g_blob_state.py_old, &min_x, &min_y, &max_x, &max_y);
            update_eyes_bounds(prev_blob_x, prev_blob_y, face_dir_x, face_dir_y,
                               prev_speed, prev_face_phase, &min_x, &min_y, &max_x, &max_y);
            update_mouth_bounds(prev_blob_x, prev_blob_y, face_dir_x, face_dir_y,
                                prev_speed, prev_face_phase, &min_x, &min_y, &max_x, &max_y);
        }

        min_x = clamp_i16(min_x - DIRTY_MARGIN, 0, SCREEN_W - 1);
        min_y = clamp_i16(min_y - DIRTY_MARGIN, 0, SCREEN_H - 1);
        max_x = clamp_i16(max_x + DIRTY_MARGIN, 0, SCREEN_W - 1);
        max_y = clamp_i16(max_y + DIRTY_MARGIN, 0, SCREEN_H - 1);

        if (!g_blob_state.first_frame)
        {
            draw_blob_glow(prev_blob_x, prev_blob_y, g_blob_state.px_old, g_blob_state.py_old, BG_COLOR);
            if (BLOB_FILL_ENABLED)
            {
                draw_blob_fill(g_blob_state.px_old, g_blob_state.py_old, BG_COLOR);
            }
            draw_blob_outline(g_blob_state.px_old, g_blob_state.py_old, BG_COLOR);
            draw_blob_eyes(prev_blob_x, prev_blob_y, face_dir_x, face_dir_y, prev_speed, prev_face_phase, BG_COLOR);
            draw_blob_mouth(prev_blob_x, prev_blob_y, face_dir_x, face_dir_y, prev_speed, prev_face_phase, BG_COLOR);
        }

        draw_blob_glow(g_blob_state.blob_x, g_blob_state.blob_y, px_new, py_new, OUTLINE_COLOR);
        if (BLOB_FILL_ENABLED)
        {
            draw_blob_fill(px_new, py_new, BLOB_COLOR);
        }
        draw_blob_outline(px_new, py_new, OUTLINE_COLOR);
        draw_blob_eyes(g_blob_state.blob_x, g_blob_state.blob_y, face_dir_x, face_dir_y,
                       speed_now, g_blob_state.phase_t, EYE_COLOR);
        draw_blob_mouth(g_blob_state.blob_x, g_blob_state.blob_y, face_dir_x, face_dir_y,
                        speed_now, g_blob_state.phase_t, MOUTH_COLOR);

        waveshare_native_present_window(min_x, min_y, max_x, max_y);
    }

    memcpy(g_blob_state.px_old, px_new, sizeof(px_new));
    memcpy(g_blob_state.py_old, py_new, sizeof(py_new));
    g_blob_state.first_frame = false;
    g_blob_state.face_phase_prev = g_blob_state.phase_t;

    g_blob_state.phase_t += 0.08f;
    delay(16);
}
