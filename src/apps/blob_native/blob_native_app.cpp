#include "apps/blob_native/blob_native_app.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "apps/blob_native/blob_native_blob.h"
#include "apps/blob_native/blob_native_face.h"
#include "apps/blob_native/blob_native_overlay.h"
#include "apps/blob_native/blob_native_state.h"
#include "platform/waveshare_native_board.h"

using namespace blob_native;

static_assert(!BLOB_STABLE_PROFILE_LOCKED || BLOB_RAW_BLOB_MODE,
              "Stable profile lock requires BLOB_RAW_BLOB_MODE=true");

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
}

void blob_native_app_loop()
{
    const uint32_t frame_start_us = micros();
    static uint16_t overlay_fps = 0;
    static uint16_t overlay_frame_count = 0;
    static uint32_t overlay_window_start_ms = 0;

    if (waveshare_native_framebuffer() == NULL)
    {
        delay(50);
        return;
    }

    static float t = 0.0f;
    static bool first = true;
    static float blob_x = CENTER_X;
    static float blob_y = CENTER_Y;
    static float vel_x = 0.0f;
    static float vel_y = 0.0f;
    static float prev_cx = CENTER_X;
    static float prev_cy = CENTER_Y;
    static float prev_speed = 0.0f;
    static int16_t prev_px[POINTS] = {0};
    static int16_t prev_py[POINTS] = {0};
    uint16_t *fb = waveshare_native_framebuffer();
    if (fb == NULL)
    {
        delay(16);
        return;
    }

    float cx = CENTER_X + cosf(t * 0.7f) * 24.0f;
    float cy = CENTER_Y + sinf(t * 0.9f) * 18.0f;
    float vx = cosf(t * 1.2f) * 0.8f;
    float vy = sinf(t * 1.1f) * 0.8f;

    if (BLOB_RAW_BLOB_USE_IMU)
    {
        float tilt_x = 0.0f;
        float tilt_y = 0.0f;
        if (waveshare_native_read_tilt(&tilt_x, &tilt_y))
        {
            const float target_x = CENTER_X + tilt_y * (CENTER_X - BLOB_RADIUS - 6.0f);
            const float target_y = CENTER_Y - tilt_x * (CENTER_Y - BLOB_RADIUS - 6.0f);

            vel_x = vel_x * 0.75f + (target_x - blob_x) * 0.15f;
            vel_y = vel_y * 0.75f + (target_y - blob_y) * 0.15f;
            blob_x += vel_x;
            blob_y += vel_y;

            cx = blob_x;
            cy = blob_y;
            vx = vel_x;
            vy = vel_y;
        }
    }

    const float speed = sqrtf(vx * vx + vy * vy);

    g_blob_state.phase_t = t;
    int16_t px[POINTS];
    int16_t py[POINTS];
    compute_blob(cx, cy, vx, vy, px, py);

    int16_t min_x = SCREEN_W - 1;
    int16_t min_y = SCREEN_H - 1;
    int16_t max_x = 0;
    int16_t max_y = 0;
    update_bounds(px, py, &min_x, &min_y, &max_x, &max_y);
    update_glow_bounds(cx, cy, px, py, &min_x, &min_y, &max_x, &max_y);
    update_eyes_bounds(cx, cy, FACE_DIR_X, FACE_DIR_Y, speed, t,
                       EyesAnimState::Idle, &min_x, &min_y, &max_x, &max_y);
    update_mouth_bounds(cx, cy, FACE_DIR_X, FACE_DIR_Y, speed, t,
                        MouthAnimState::Neutral, &min_x, &min_y, &max_x, &max_y);

    if (!first)
    {
        update_bounds(prev_px, prev_py, &min_x, &min_y, &max_x, &max_y);
        update_glow_bounds(prev_cx, prev_cy, prev_px, prev_py, &min_x, &min_y, &max_x, &max_y);
        update_eyes_bounds(prev_cx, prev_cy, FACE_DIR_X, FACE_DIR_Y, prev_speed, t - 0.08f,
                           EyesAnimState::Idle, &min_x, &min_y, &max_x, &max_y);
        update_mouth_bounds(prev_cx, prev_cy, FACE_DIR_X, FACE_DIR_Y, prev_speed, t - 0.08f,
                            MouthAnimState::Neutral, &min_x, &min_y, &max_x, &max_y);
    }

    min_x = clamp_i16(min_x - DIRTY_MARGIN, 0, SCREEN_W - 1);
    min_y = clamp_i16(min_y - DIRTY_MARGIN, 0, SCREEN_H - 1);
    max_x = clamp_i16(max_x + DIRTY_MARGIN, 0, SCREEN_W - 1);
    max_y = clamp_i16(max_y + DIRTY_MARGIN, 0, SCREEN_H - 1);

    bool overlay_redraw = false;
    if (BLOB_PERF_OVERLAY_ENABLED)
    {
        overlay_frame_count++;
        const uint32_t now_ms = millis();
        if (overlay_window_start_ms == 0)
        {
            overlay_window_start_ms = now_ms;
            overlay_redraw = true;
        }

        const uint32_t elapsed_ms = now_ms - overlay_window_start_ms;
        if (elapsed_ms >= BLOB_PERF_OVERLAY_UPDATE_MS)
        {
            overlay_fps = (uint16_t)((overlay_frame_count * 1000u) / elapsed_ms);
            overlay_frame_count = 0;
            overlay_window_start_ms = now_ms;
            overlay_redraw = true;
        }

        if (overlay_redraw)
        {
            overlay_expand_dirty_bounds(&min_x, &min_y, &max_x, &max_y);
        }
    }

    const uint16_t black_swapped = (uint16_t)((BLACK << 8) | (BLACK >> 8));
    for (int y = min_y; y <= max_y; y++)
    {
        uint16_t *row = &fb[y * SCREEN_W + min_x];
        for (int x = min_x; x <= max_x; x++)
        {
            *row++ = black_swapped;
        }
    }

    draw_blob_glow(cx, cy, px, py, OUTLINE_COLOR);
    if (BLOB_FILL_ENABLED)
    {
        draw_blob_fill(px, py, BLOB_COLOR);
    }
    draw_blob_outline(px, py, OUTLINE_COLOR);
    draw_blob_eyes(cx, cy, FACE_DIR_X, FACE_DIR_Y, speed, t, EyesAnimState::Idle, EYE_COLOR);
    draw_blob_mouth(cx, cy, FACE_DIR_X, FACE_DIR_Y, speed, t, MouthAnimState::Neutral, MOUTH_COLOR);

    if (BLOB_PERF_OVERLAY_ENABLED && overlay_redraw)
    {
        overlay_draw(fb, overlay_fps, max_x - min_x + 1, max_y - min_y + 1);
    }

    if (BLOB_FORCE_FULL_PRESENT)
    {
        waveshare_native_present_full();
    }
    else
    {
        waveshare_native_present_window(min_x, min_y, max_x, max_y);
    }

    memcpy(prev_px, px, sizeof(px));
    memcpy(prev_py, py, sizeof(py));
    prev_cx = cx;
    prev_cy = cy;
    prev_speed = speed;
    first = false;

    t += 0.08f;

    // Cap to ~60 FPS without forcing additional delay on already-heavy frames.
    const uint32_t frame_elapsed_us = micros() - frame_start_us;
    constexpr uint32_t FRAME_TARGET_US = 16667;
    if (frame_elapsed_us < FRAME_TARGET_US)
    {
        delayMicroseconds(FRAME_TARGET_US - frame_elapsed_us);
    }
}
