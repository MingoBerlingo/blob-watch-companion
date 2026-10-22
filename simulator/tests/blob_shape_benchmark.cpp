#include <Arduino.h>
#include <cmath>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include "apps/blob_native/shared_state.h"
#include "apps/blob_native/blob/shape.h"
using namespace blob_native;
    void reference_blob(float cx, float cy, float vx, float vy, float phase_t, int16_t *px, int16_t *py)
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


int main() {
    int16_t ax[POINTS], ay[POINTS], bx[POINTS], by[POINTS];
    int max_error = 0;
    const float speeds[] = {0.0f, 0.001f, 0.8f, 3.0f, 6.66f, 6.67f, 12.0f, 30.0f};
    for (int frame = 0; frame < 20000; ++frame) {
        const float phase = frame * 0.08f;
        const float speed = speeds[frame % 8];
        const float vx = std::sin(phase) * speed;
        const float vy = std::cos(phase) * speed;
        compute_blob(120, 120, vx, vy, phase, ax, ay);
        reference_blob(120, 120, vx, vy, phase, bx, by);
        for (int i=0; i<POINTS; ++i) {
            max_error = std::max(max_error, std::abs(ax[i]-bx[i]));
            max_error = std::max(max_error, std::abs(ay[i]-by[i]));
        }
    }
    if (max_error > 1) { std::cerr << "Contour deviation " << max_error << "px\n"; return 1; }
    volatile int checksum = 0;
    const auto bench = [&](auto function) {
        const auto start = std::chrono::steady_clock::now();
        for (int i=0; i<100000; ++i) {
            function(120,120,0.8f,0.6f,(i%10000)*0.08f,ax,ay);
            checksum += ax[i%POINTS];
        }
        return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    };
    const auto before=bench(reference_blob), after=bench(compute_blob);
    std::cout << "Max contour deviation: " << max_error << "px; 100000 contours: reference "
              << before << "ms, current " << after << "ms; speedup " << before/after << "x\n";
}
