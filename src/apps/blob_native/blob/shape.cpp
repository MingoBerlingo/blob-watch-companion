#include "apps/blob_native/blob/shape.h"

#include <Arduino.h>
#include <math.h>

#include "apps/blob_native/shared_state.h"
#include "apps/blob_native/ui/draw.h"

namespace blob_native
{

    static int insertion_sort_int(int *values, int count)
    {
        for (int i = 1; i < count; i++)
        {
            const int key = values[i];
            int j = i - 1;
            while (j >= 0 && values[j] > key)
            {
                values[j + 1] = values[j];
                j--;
            }
            values[j + 1] = key;
        }
        return count;
    }

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
            draw_line(px[i], py[i], px[next], py[next], color);
        }
    }

    void draw_blob_fill(const int16_t *px, const int16_t *py, uint16_t color)
    {
        int16_t min_x = SCREEN_W - 1;
        int16_t min_y = SCREEN_H - 1;
        int16_t max_x = 0;
        int16_t max_y = 0;
        update_bounds(px, py, &min_x, &min_y, &max_x, &max_y);

        min_x = clamp_i16(min_x, 0, SCREEN_W - 1);
        max_x = clamp_i16(max_x, 0, SCREEN_W - 1);
        min_y = clamp_i16(min_y, 0, SCREEN_H - 1);
        max_y = clamp_i16(max_y, 0, SCREEN_H - 1);

        int x_intersections[POINTS];

        for (int y = min_y; y <= max_y; y++)
        {
            int count = 0;
            for (int i = 0; i < POINTS; i++)
            {
                const int next = (i + 1) % POINTS;
                const int x1 = px[i];
                const int y1 = py[i];
                const int x2 = px[next];
                const int y2 = py[next];

                if (y1 == y2)
                {
                    continue;
                }

                const int y_min = (y1 < y2) ? y1 : y2;
                const int y_max = (y1 > y2) ? y1 : y2;
                if (y < y_min || y >= y_max)
                {
                    continue;
                }

                const int dx = x2 - x1;
                const int dy = y2 - y1;
                const int x = x1 + (dx * (y - y1)) / dy;
                x_intersections[count++] = x;
            }

            if (count < 2)
            {
                continue;
            }

            insertion_sort_int(x_intersections, count);

            for (int i = 0; i + 1 < count; i += 2)
            {
                int x_start = x_intersections[i];
                int x_end = x_intersections[i + 1];
                if (x_start > x_end)
                {
                    const int tmp = x_start;
                    x_start = x_end;
                    x_end = tmp;
                }

                x_start = clamp_i16(x_start, min_x, max_x);
                x_end = clamp_i16(x_end, min_x, max_x);
                if (x_start <= x_end)
                {
                    draw_line(x_start, y, x_end, y, color);
                }
            }
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
                draw_line(glow_px[i], glow_py[i], glow_px[next], glow_py[next], layer_color);
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