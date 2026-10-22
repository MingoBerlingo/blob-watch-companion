#include "apps/blob_native/blob/soft_body.h"

#include <math.h>

namespace blob_native
{
    namespace
    {
        constexpr uint32_t kStepMs = 8;
        constexpr uint32_t kMaxCatchupMs = 64;
        constexpr float kDt = kStepMs / 1000.0f;
        constexpr float kShapeStiffness = 320.0f;
        constexpr float kShapeDamping = 12.0f;
        constexpr float kEdgeStiffness = 240.0f;
        constexpr float kEdgeDamping = 2.5f;
        constexpr float kAirDrag = 0.9f;
        constexpr float kBounce = 0.2f;
        constexpr float kContactFriction = 0.04f;
        constexpr float kPi = 3.14159265358979323846f;

        void measure(SoftBody &body)
        {
            body.center_x = body.center_y = body.velocity_x = body.velocity_y = 0;
            for (const auto &p : body.points)
            {
                body.center_x += p.x;
                body.center_y += p.y;
                body.velocity_x += p.vx;
                body.velocity_y += p.vy;
            }
            body.center_x /= SOFT_BODY_POINTS;
            body.center_y /= SOFT_BODY_POINTS;
            body.velocity_x /= SOFT_BODY_POINTS;
            body.velocity_y /= SOFT_BODY_POINTS;
        }

        float safe_acceleration(float value)
        {
            if (!isfinite(value)) return 0;
            return value < -1200 ? -1200 : (value > 1200 ? 1200 : value);
        }

        void step(SoftBody &body, float ax, float ay)
        {
            measure(body);
            float dot = 0, cross = 0, inertia = 0, angular_momentum = 0;
            for (int i = 0; i < SOFT_BODY_POINTS; ++i)
            {
                const auto &p = body.points[i];
                const float x = p.x - body.center_x, y = p.y - body.center_y;
                dot += x * body.rest_x[i] + y * body.rest_y[i];
                cross += body.rest_x[i] * y - body.rest_y[i] * x;
                inertia += x * x + y * y;
                angular_momentum += x * (p.vy - body.velocity_y) - y * (p.vx - body.velocity_x);
            }
            // Normalize the best-fit rotation directly: no per-step atan/sin/cos.
            const float length = sqrtf(dot * dot + cross * cross);
            const float cosine = length > 0.0001f ? dot / length : 1;
            const float sine = length > 0.0001f ? cross / length : 0;
            const float omega = inertia > 0.0001f ? angular_momentum / inertia : 0;
            float fx[SOFT_BODY_POINTS], fy[SOFT_BODY_POINTS];
            for (int i = 0; i < SOFT_BODY_POINTS; ++i)
            {
                const auto &p = body.points[i];
                const float goal_x = body.center_x + cosine * body.rest_x[i] - sine * body.rest_y[i];
                const float goal_y = body.center_y + sine * body.rest_x[i] + cosine * body.rest_y[i];
                const float rigid_vx = body.velocity_x - omega * (p.y - body.center_y);
                const float rigid_vy = body.velocity_y + omega * (p.x - body.center_x);
                // Dampen deformation without damping rigid translation/rotation.
                fx[i] = ax + kShapeStiffness * (goal_x - p.x) - kShapeDamping * (p.vx - rigid_vx);
                fy[i] = ay + kShapeStiffness * (goal_y - p.y) - kShapeDamping * (p.vy - rigid_vy);
            }
            for (int i = 0; i < SOFT_BODY_POINTS; ++i)
            {
                const int j = (i + 1) % SOFT_BODY_POINTS;
                const auto &a = body.points[i], &b = body.points[j];
                const float dx = b.x - a.x, dy = b.y - a.y;
                const float distance = sqrtf(dx * dx + dy * dy);
                if (distance < 0.0001f) continue;
                const float nx = dx / distance, ny = dy / distance;
                const float relative_speed = (b.vx - a.vx) * nx + (b.vy - a.vy) * ny;
                const float force = kEdgeStiffness * (distance - body.rest_edge[i]) + kEdgeDamping * relative_speed;
                fx[i] += force * nx;
                fy[i] += force * ny;
                fx[j] -= force * nx;
                fy[j] -= force * ny;
            }
            for (int i = 0; i < SOFT_BODY_POINTS; ++i)
            {
                auto &p = body.points[i];
                p.vx = (p.vx + fx[i] * kDt) / (1 + kAirDrag * kDt);
                p.vy = (p.vy + fy[i] * kDt) / (1 + kAirDrag * kDt);
                p.x += p.vx * kDt;
                p.y += p.vy * kDt;
                const float dx = p.x - body.arena_x, dy = p.y - body.arena_y;
                const float squared = dx * dx + dy * dy;
                if (squared <= body.arena_radius * body.arena_radius) continue;
                const float inverse = 1 / sqrtf(squared);
                const float nx = dx * inverse, ny = dy * inverse;
                p.x = body.arena_x + nx * body.arena_radius;
                p.y = body.arena_y + ny * body.arena_radius;
                const float outward_speed = p.vx * nx + p.vy * ny;
                // A point already moving inward must not bounce back into the wall.
                if (outward_speed > 0)
                {
                    p.vx -= (1 + kBounce) * outward_speed * nx;
                    p.vy -= (1 + kBounce) * outward_speed * ny;
                }
                const float normal_speed = p.vx * nx + p.vy * ny;
                p.vx -= kContactFriction * (p.vx - normal_speed * nx);
                p.vy -= kContactFriction * (p.vy - normal_speed * ny);
            }
        }

        float interpolate(float a, float b, float c, float d, float t)
        {
            // Closed Catmull-Rom contour: drawing resolution is independent of physics.
            return 0.5f * (2 * b + (-a + c) * t +
                           (2 * a - 5 * b + 4 * c - d) * t * t +
                           (-a + 3 * b - 3 * c + d) * t * t * t);
        }
    }

    void soft_body_reset(SoftBody &body, float center_x, float center_y, float radius, float arena_radius)
    {
        body = {};
        body.arena_x = center_x;
        body.arena_y = center_y;
        body.arena_radius = arena_radius;
        for (int i = 0; i < SOFT_BODY_POINTS; ++i)
        {
            // Five tips and five valleys. Start with a tip pointing upward.
            const float angle = -kPi * 0.5f + 2 * kPi * i / SOFT_BODY_POINTS;
            const float point_radius = (i % 2 == 0) ? radius : radius * 0.62f;
            body.rest_x[i] = point_radius * cosf(angle);
            body.rest_y[i] = point_radius * sinf(angle);
            body.points[i] = {center_x + body.rest_x[i], center_y + body.rest_y[i], 0, 0};
        }
        for (int i = 0; i < SOFT_BODY_POINTS; ++i)
        {
            const int j = (i + 1) % SOFT_BODY_POINTS;
            const float dx = body.rest_x[j] - body.rest_x[i], dy = body.rest_y[j] - body.rest_y[i];
            body.rest_edge[i] = sqrtf(dx * dx + dy * dy);
        }
        measure(body);
    }

    void soft_body_impulse(SoftBody &body, float velocity_x, float velocity_y)
    {
        if (!isfinite(velocity_x) || !isfinite(velocity_y)) return;
        float vx=body.velocity_x+velocity_x, vy=body.velocity_y+velocity_y;
        const float speed=sqrtf(vx*vx+vy*vy);
        if (speed>400) { vx*=400/speed; vy*=400/speed; }
        const float dx=vx-body.velocity_x, dy=vy-body.velocity_y;
        for (auto &point:body.points) { point.vx+=dx; point.vy+=dy; }
        body.velocity_x=vx; body.velocity_y=vy;
    }

    void soft_body_update(SoftBody &body, uint32_t elapsed_ms, float acceleration_x, float acceleration_y)
    {
        // At most eight fixed steps: pauses/debugger stalls cannot cause an
        // unbounded catch-up loop or a single unstable integration step.
        body.remainder_ms += elapsed_ms > kMaxCatchupMs ? kMaxCatchupMs : elapsed_ms;
        const float ax = safe_acceleration(acceleration_x), ay = safe_acceleration(acceleration_y);
        while (body.remainder_ms >= kStepMs)
        {
            step(body, ax, ay);
            body.remainder_ms -= kStepMs;
        }
        measure(body);
    }

    void soft_body_outline(const SoftBody &body, int16_t *px, int16_t *py, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            const float sample = (float)i * SOFT_BODY_POINTS / count;
            const int b = (int)sample;
            const auto &a = body.points[(b + SOFT_BODY_POINTS - 1) % SOFT_BODY_POINTS];
            const auto &p = body.points[b % SOFT_BODY_POINTS];
            const auto &c = body.points[(b + 1) % SOFT_BODY_POINTS];
            const auto &d = body.points[(b + 2) % SOFT_BODY_POINTS];
            const float t = sample - b;
            float x = interpolate(a.x, p.x, c.x, d.x, t);
            float y = interpolate(a.y, p.y, c.y, d.y, t);
            const float dx = x - body.arena_x, dy = y - body.arena_y;
            const float squared = dx * dx + dy * dy;
            if (squared > body.arena_radius * body.arena_radius)
            {
                const float factor = body.arena_radius / sqrtf(squared);
                x = body.arena_x + dx * factor;
                y = body.arena_y + dy * factor;
            }
            px[i] = (int16_t)x;
            py[i] = (int16_t)y;
        }
    }
}
