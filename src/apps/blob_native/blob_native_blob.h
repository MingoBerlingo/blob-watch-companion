#ifndef BLOB_NATIVE_BLOB_H
#define BLOB_NATIVE_BLOB_H

#include <stdint.h>

namespace blob_native
{

    void compute_blob(float cx, float cy, float vx, float vy, int16_t *px, int16_t *py);
    void update_bounds(const int16_t *px, const int16_t *py, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);

    void scale_blob_points(float cx, float cy, float scale, const int16_t *px_in, const int16_t *py_in, int16_t *px_out, int16_t *py_out);
    void draw_blob_outline(const int16_t *px, const int16_t *py, uint16_t color);
    void draw_blob_glow(float cx, float cy, const int16_t *px, const int16_t *py, uint16_t base_color);
    void update_glow_bounds(float cx, float cy, const int16_t *px, const int16_t *py, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);

} // namespace blob_native

#endif