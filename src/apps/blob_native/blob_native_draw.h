#ifndef BLOB_NATIVE_DRAW_H
#define BLOB_NATIVE_DRAW_H

#include <stdint.h>

namespace blob_native
{

    void draw_pixel(int x, int y, uint16_t color);
    void draw_line(int x0, int y0, int x1, int y1, uint16_t color);
    void draw_circle(int cx, int cy, int radius, uint16_t color, bool filled);

} // namespace blob_native

#endif
