#include "apps/blob_native/blob/shape.h"

#include <Arduino.h>
#include <math.h>

#include "apps/blob_native/shared_state.h"

namespace blob_native
{

    namespace
    {
        struct Direction
        {
            float x, y;
            float sin3, cos3, sin5, cos5, sin7, cos7;
        };

        struct ContourBasis
        {
            Direction points[POINTS];
            ContourBasis()
            {
                for (int i = 0; i < POINTS; ++i)
                {
                    const float theta = (2.0f * PI * i) / POINTS;
                    points[i] = {cosf(theta), sinf(theta),
                                 sinf(3 * theta), cosf(3 * theta),
                                 sinf(5 * theta), cosf(5 * theta),
                                 sinf(7 * theta), cosf(7 * theta)};
                }
            }
        };
    }

    void compute_blob(float cx, float cy, float vx, float vy, float phase_t, int16_t *px, int16_t *py)
    {
        // Angular samples never change. Reuse them and expand sin(a + b),
        // leaving only six phase-dependent trigonometric calls per contour.
        static const ContourBasis basis;
        const float sin1 = sinf(phase_t * 1.3f), cos1 = cosf(phase_t * 1.3f);
        const float sin2 = sinf(phase_t * 0.9f), cos2 = cosf(phase_t * 0.9f);
        const float sin3 = sinf(phase_t * 0.4f), cos3 = cosf(phase_t * 0.4f);
        const float speed = sqrtf(vx * vx + vy * vy);
        const float deformation = speed > 0 ? 1.5f * constrain(speed * 1.2f, 0.0f, 8.0f) / speed : 0;

        for (int i = 0; i < POINTS; ++i)
        {
            const auto &d = basis.points[i];
            float r = BLOB_RADIUS + BLOB_WAVE_AMP_1 * (d.sin3 * cos1 + d.cos3 * sin1) +
                      BLOB_WAVE_AMP_2 * (d.sin5 * cos2 - d.cos5 * sin2) +
                      BLOB_WAVE_AMP_3 * (d.sin7 * cos3 + d.cos7 * sin3);
            // Project velocity onto the radial direction; avoids atan2 and
            // two cosine calls per point for the same directional squish.
            r += deformation * (d.x * vx + d.y * vy);
            px[i] = (int16_t)(cx + r * d.x);
            py[i] = (int16_t)(cy + r * d.y);
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

} // namespace blob_native
