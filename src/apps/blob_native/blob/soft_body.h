#ifndef BLOB_NATIVE_SOFT_BODY_H
#define BLOB_NATIVE_SOFT_BODY_H

#include <stdint.h>

namespace blob_native
{
    constexpr int SOFT_BODY_POINTS = 10;

    struct SoftBodyPoint
    {
        float x, y, vx, vy;
    };

    struct SoftBody
    {
        SoftBodyPoint points[SOFT_BODY_POINTS];
        float rest_x[SOFT_BODY_POINTS], rest_y[SOFT_BODY_POINTS];
        float rest_edge[SOFT_BODY_POINTS];
        float center_x, center_y, velocity_x, velocity_y;
        float arena_x, arena_y, arena_radius;
        uint32_t remainder_ms;
    };

    void soft_body_reset(SoftBody &body, float center_x, float center_y, float radius, float arena_radius);
    void soft_body_impulse(SoftBody &body, float velocity_x, float velocity_y);
    void soft_body_update(SoftBody &body, uint32_t elapsed_ms, float acceleration_x, float acceleration_y);
    void soft_body_outline(const SoftBody &body, int16_t *px, int16_t *py, int count);
}

#endif
