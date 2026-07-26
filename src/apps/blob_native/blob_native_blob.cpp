#include "apps/blob_native/blob_native_blob.h"

#include <Arduino.h>
#include <math.h>

#include "GUI_Paint.h"
#include "apps/blob_native/blob_native_state.h"

namespace blob_native
{

    void compute_blob(float cx, float cy, float vx, float vy, int16_t *px, int16_t *py)
    {
        const float speed = sqrtf(vx * vx + vy * vy);
        const float move_angle = atan2f(vy, vx);

        for (int i = 0; i < POINTS; i++)
        {
            const float theta = (2.0f * PI * i) / POINTS;

            float r = BLOB_RADIUS + BLOB_WAVE_AMP_1 * sinf(3.0f * theta + g_blob_state.phase_t * 1.3f) +
                      BLOB_WAVE_AMP_2 * sinf(5.0f * theta - g_blob_state.phase_t * 0.9f) +
                      BLOB_WAVE_AMP_3 * sinf(7.0f * theta + g_blob_state.phase_t * 0.4f);

            const float squish = constrain(speed * 1.2f, 0.0f, 8.0f);
            r += squish * cosf(theta - move_angle);
            r -= squish * 0.5f * cosf(theta - move_angle + PI);

            px[i] = (int16_t)(cx + r * cosf(theta));
            py[i] = (int16_t)(cy + r * sinf(theta));
        }
    }

    void update_bounds(const int16_t *px, const int16_t *py, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
    {
        for (int i = 0; i < POINTS; i++)
        {
            if (px[i] < *min_x)
                *min_x = px[i];
            if (py[i] < *min_y)
                *min_y = py[i];
            if (px[i] > *max_x)
                *max_x = px[i];
            if (py[i] > *max_y)
                *max_y = py[i];
        }
    }

    void scale_blob_points(float cx, float cy, float scale, const int16_t *px_in, const int16_t *py_in, int16_t *px_out, int16_t *py_out)
    {
        for (int i = 0; i < POINTS; i++)
        {
            px_out[i] = (int16_t)(cx + (px_in[i] - cx) * scale);
            py_out[i] = (int16_t)(cy + (py_in[i] - cy) * scale);
        }
    }

    void draw_blob_outline(const int16_t *px, const int16_t *py, uint16_t color)
    {
        for (int i = 0; i < POINTS; i++)
        {
            const int next = (i + 1) % POINTS;
            Paint_DrawLine(px[i], py[i], px[next], py[next], color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        }
    }

    void draw_blob_glow(float cx, float cy, const int16_t *px, const int16_t *py, uint16_t base_color)
    {
        int16_t glow_px[POINTS];
        int16_t glow_py[POINTS];

        for (int layer = 0; layer < GLOW_LAYER_COUNT; layer++)
        {
            scale_blob_points(cx, cy, GLOW_LAYER_SCALE[layer], px, py, glow_px, glow_py);

            for (int i = 0; i < POINTS; i++)
            {
                const int next = (i + 1) % POINTS;
                const uint16_t layer_color = (base_color == BG_COLOR) ? BG_COLOR : GLOW_LAYER_COLOR[layer];
                Paint_DrawLine(glow_px[i], glow_py[i], glow_px[next], glow_py[next], layer_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            }
        }
    }

    void update_glow_bounds(float cx, float cy, const int16_t *px, const int16_t *py, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
    {
        int16_t glow_px[POINTS];
        int16_t glow_py[POINTS];

        scale_blob_points(cx, cy, GLOW_LAYER_SCALE[0], px, py, glow_px, glow_py);
        update_bounds(glow_px, glow_py, min_x, min_y, max_x, max_y);
    }

} // namespace blob_native