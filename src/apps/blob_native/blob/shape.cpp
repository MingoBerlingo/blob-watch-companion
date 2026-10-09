#include "apps/blob_native/blob/shape.h"

#include <Arduino.h>
#include <math.h>

#include "apps/blob_native/shared_state.h"

namespace blob_native
{

    void compute_blob(float cx, float cy, float vx, float vy, float phase_t, int16_t *px, int16_t *py)
    {
        const float speed = sqrtf(vx * vx + vy * vy);
        const float move_angle = atan2f(vy, vx);

        for (int i = 0; i < POINTS; i++)
        {
            const float theta = (2.0f * PI * i) / POINTS;

            float r = BLOB_RADIUS + BLOB_WAVE_AMP_1 * sinf(3.0f * theta + phase_t * 1.3f) +
                      BLOB_WAVE_AMP_2 * sinf(5.0f * theta - phase_t * 0.9f) +
                      BLOB_WAVE_AMP_3 * sinf(7.0f * theta + phase_t * 0.4f);

            const float squish = constrain(speed * 1.2f, 0.0f, 8.0f);
            r += squish * cosf(theta - move_angle);
            r -= squish * 0.5f * cosf(theta - move_angle + PI);

            px[i] = (int16_t)(cx + r * cosf(theta));
            py[i] = (int16_t)(cy + r * sinf(theta));
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
