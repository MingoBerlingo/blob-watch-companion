#include "apps/blob_native/blob_native_app.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

#include "apps/blob_native/blob_native_blob.h"
#include "apps/blob_native/blob_native_face.h"
#include "apps/blob_native/blob_native_state.h"
#include "platform/waveshare_native_board.h"

using namespace blob_native;

static_assert(!BLOB_STABLE_PROFILE_LOCKED || BLOB_RAW_BLOB_MODE,
              "Stable profile lock requires BLOB_RAW_BLOB_MODE=true");

namespace
{

    constexpr int16_t kOverlayScale = 1;
    constexpr int16_t kOverlayW = 80;
    constexpr int16_t kOverlayH = 16;

    static inline uint16_t to_panel_rgb565(uint16_t color)
    {
        return (uint16_t)((color << 8) | (color >> 8));
    }

    static inline void fb_put_pixel(uint16_t *fb, int x, int y, uint16_t color_swapped)
    {
        if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H)
        {
            return;
        }
        fb[y * SCREEN_W + x] = color_swapped;
    }

    static inline void fb_put_block(uint16_t *fb, int x, int y, int scale, uint16_t color_swapped)
    {
        for (int dy = 0; dy < scale; dy++)
        {
            for (int dx = 0; dx < scale; dx++)
            {
                fb_put_pixel(fb, x + dx, y + dy, color_swapped);
            }
        }
    }

    static inline void draw_hline(uint16_t *fb, int x0, int x1, int y, uint16_t color_swapped)
    {
        if (y < 0 || y >= SCREEN_H)
        {
            return;
        }

        if (x0 > x1)
        {
            const int t = x0;
            x0 = x1;
            x1 = t;
        }

        if (x1 < 0 || x0 >= SCREEN_W)
        {
            return;
        }

        if (x0 < 0)
        {
            x0 = 0;
        }
        if (x1 >= SCREEN_W)
        {
            x1 = SCREEN_W - 1;
        }

        uint16_t *row = &fb[y * SCREEN_W + x0];
        for (int x = x0; x <= x1; x++)
        {
            *row++ = color_swapped;
        }
    }

    static void draw_3x5_glyph(uint16_t *fb, int x, int y, const uint8_t glyph[5], int scale, uint16_t color_swapped)
    {
        for (int row = 0; row < 5; row++)
        {
            const uint8_t bits = glyph[row];
            for (int col = 0; col < 3; col++)
            {
                if (bits & (1u << (2 - col)))
                {
                    fb_put_block(fb, x + col * scale, y + row * scale, scale, color_swapped);
                }
            }
        }
    }

    static void draw_3x5_digit(uint16_t *fb, int x, int y, int digit, int scale, uint16_t color_swapped)
    {
        static const uint8_t glyphs[10][5] = {
            {0b111, 0b101, 0b101, 0b101, 0b111},
            {0b010, 0b110, 0b010, 0b010, 0b111},
            {0b111, 0b001, 0b111, 0b100, 0b111},
            {0b111, 0b001, 0b111, 0b001, 0b111},
            {0b101, 0b101, 0b111, 0b001, 0b001},
            {0b111, 0b100, 0b111, 0b001, 0b111},
            {0b111, 0b100, 0b111, 0b101, 0b111},
            {0b111, 0b001, 0b001, 0b001, 0b001},
            {0b111, 0b101, 0b111, 0b101, 0b111},
            {0b111, 0b101, 0b111, 0b001, 0b111},
        };

        if (digit < 0 || digit > 9)
        {
            return;
        }

        draw_3x5_glyph(fb, x, y, glyphs[digit], scale, color_swapped);
    }

    static void draw_3x5_char(uint16_t *fb, int x, int y, char c, int scale, uint16_t color_swapped)
    {
        static const uint8_t glyph_f[5] = {0b111, 0b100, 0b111, 0b100, 0b100};
        static const uint8_t glyph_p[5] = {0b110, 0b101, 0b110, 0b100, 0b100};
        static const uint8_t glyph_s[5] = {0b111, 0b100, 0b111, 0b001, 0b111};
        static const uint8_t glyph_w[5] = {0b101, 0b101, 0b101, 0b111, 0b101};
        static const uint8_t glyph_h[5] = {0b101, 0b101, 0b111, 0b101, 0b101};
        static const uint8_t glyph_x[5] = {0b101, 0b101, 0b010, 0b101, 0b101};

        if (c >= '0' && c <= '9')
        {
            draw_3x5_digit(fb, x, y, c - '0', scale, color_swapped);
            return;
        }

        if (c == 'F')
            draw_3x5_glyph(fb, x, y, glyph_f, scale, color_swapped);
        else if (c == 'P')
            draw_3x5_glyph(fb, x, y, glyph_p, scale, color_swapped);
        else if (c == 'S')
            draw_3x5_glyph(fb, x, y, glyph_s, scale, color_swapped);
        else if (c == 'W')
            draw_3x5_glyph(fb, x, y, glyph_w, scale, color_swapped);
        else if (c == 'H')
            draw_3x5_glyph(fb, x, y, glyph_h, scale, color_swapped);
        else if (c == 'x')
            draw_3x5_glyph(fb, x, y, glyph_x, scale, color_swapped);
    }

    static void draw_3x5_text(uint16_t *fb, int x, int y, const char *text, int scale, uint16_t color_swapped)
    {
        if (text == nullptr)
        {
            return;
        }

        int cursor_x = x;
        for (const char *p = text; *p != '\0'; p++)
        {
            if (*p != ' ')
            {
                draw_3x5_char(fb, cursor_x, y, *p, scale, color_swapped);
            }
            cursor_x += 4 * scale;
        }
    }

    static inline void append_char(char *dst, int &idx, int max_len, char c)
    {
        if (idx < (max_len - 1))
        {
            dst[idx++] = c;
        }
    }

    static void append_uint(char *dst, int &idx, int max_len, uint32_t value)
    {
        char tmp[10];
        int count = 0;
        do
        {
            tmp[count++] = (char)('0' + (value % 10));
            value /= 10;
        } while (value != 0 && count < (int)sizeof(tmp));

        for (int i = count - 1; i >= 0; i--)
        {
            append_char(dst, idx, max_len, tmp[i]);
        }
    }

    static void draw_telemetry_overlay(uint16_t *fb, uint16_t fps, int rect_w, int rect_h)
    {
        if (fb == nullptr)
        {
            return;
        }

        const uint16_t text = to_panel_rgb565(CYAN);
        char line1[22] = {0};
        int idx1 = 0;
        append_char(line1, idx1, (int)sizeof(line1), 'F');
        append_char(line1, idx1, (int)sizeof(line1), 'P');
        append_char(line1, idx1, (int)sizeof(line1), 'S');
        append_char(line1, idx1, (int)sizeof(line1), ' ');
        append_uint(line1, idx1, (int)sizeof(line1), fps);

        char line2[22] = {0};
        int idx2 = 0;
        append_char(line2, idx2, (int)sizeof(line2), 'W');
        append_char(line2, idx2, (int)sizeof(line2), ' ');
        append_uint(line2, idx2, (int)sizeof(line2), (uint32_t)rect_w);
        append_char(line2, idx2, (int)sizeof(line2), '   ');
        append_char(line2, idx2, (int)sizeof(line2), 'H');
        append_char(line2, idx2, (int)sizeof(line2), ' ');
        append_uint(line2, idx2, (int)sizeof(line2), (uint32_t)rect_h);

        draw_3x5_text(fb, BLOB_TELEMETRY_X, BLOB_TELEMETRY_Y, line1, kOverlayScale, text);
        draw_3x5_text(fb, BLOB_TELEMETRY_X, BLOB_TELEMETRY_Y + 7, line2, kOverlayScale, text);
    }

} // namespace

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
    static uint16_t telemetry_fps = 0;
    static uint16_t telemetry_frame_count = 0;
    static uint32_t telemetry_window_start_ms = 0;
    static uint32_t telemetry_last_draw_ms = 0;

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

    bool telemetry_redraw = false;
    int telemetry_rect_w = 0;
    int telemetry_rect_h = 0;

    if (BLOB_TELEMETRY_OVERLAY_ENABLED)
    {
        telemetry_frame_count++;
        const uint32_t now_ms = millis();
        if (telemetry_window_start_ms == 0)
        {
            telemetry_window_start_ms = now_ms;
            telemetry_redraw = true;
        }

        const uint32_t elapsed_ms = now_ms - telemetry_window_start_ms;
        if (elapsed_ms >= 250)
        {
            telemetry_fps = (uint16_t)((telemetry_frame_count * 1000u) / elapsed_ms);
            telemetry_frame_count = 0;
            telemetry_window_start_ms = now_ms;
            telemetry_redraw = true;
        }

        if (!telemetry_redraw && (telemetry_last_draw_ms == 0 || (now_ms - telemetry_last_draw_ms) >= 500))
        {
            telemetry_redraw = true;
        }

        if (telemetry_redraw)
        {
            telemetry_last_draw_ms = now_ms;
            const int16_t ox0 = clamp_i16(BLOB_TELEMETRY_X - 1, 0, SCREEN_W - 1);
            const int16_t oy0 = clamp_i16(BLOB_TELEMETRY_Y - 1, 0, SCREEN_H - 1);
            const int16_t ox1 = clamp_i16(BLOB_TELEMETRY_X + kOverlayW, 0, SCREEN_W - 1);
            const int16_t oy1 = clamp_i16(BLOB_TELEMETRY_Y + kOverlayH, 0, SCREEN_H - 1);
            if (ox0 < min_x)
                min_x = ox0;
            if (oy0 < min_y)
                min_y = oy0;
            if (ox1 > max_x)
                max_x = ox1;
            if (oy1 > max_y)
                max_y = oy1;
        }

        telemetry_rect_w = max_x - min_x + 1;
        telemetry_rect_h = max_y - min_y + 1;
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

    if (BLOB_TELEMETRY_OVERLAY_ENABLED && telemetry_redraw)
    {
        draw_telemetry_overlay(fb, telemetry_fps, telemetry_rect_w, telemetry_rect_h);
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
