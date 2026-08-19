#include "apps/blob_native/ui/draw.h"

#include <stdlib.h>

#include "apps/blob_native/shared_state.h"
#include "platform/waveshare_native_board.h"

namespace blob_native
{

    static inline uint16_t to_panel_rgb565(uint16_t color)
    {
        return (uint16_t)((color << 8) | (color >> 8));
    }

    static inline void put_pixel(uint16_t *fb, int x, int y, uint16_t color_swapped)
    {
        if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H)
        {
            return;
        }

        fb[y * SCREEN_W + x] = color_swapped;
    }

    void draw_pixel(int x, int y, uint16_t color)
    {
        uint16_t *fb = waveshare_native_framebuffer();
        if (fb == nullptr)
        {
            return;
        }

        put_pixel(fb, x, y, to_panel_rgb565(color));
    }

    void draw_line(int x0, int y0, int x1, int y1, uint16_t color)
    {
        uint16_t *fb = waveshare_native_framebuffer();
        if (fb == nullptr)
        {
            return;
        }

        const uint16_t color_swapped = to_panel_rgb565(color);
        int dx = abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true)
        {
            put_pixel(fb, x0, y0, color_swapped);
            if (x0 == x1 && y0 == y1)
            {
                break;
            }

            const int e2 = 2 * err;
            if (e2 >= dy)
            {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }

    void draw_circle(int cx, int cy, int radius, uint16_t color, bool filled)
    {
        uint16_t *fb = waveshare_native_framebuffer();
        if (fb == nullptr)
        {
            return;
        }

        const uint16_t color_swapped = to_panel_rgb565(color);

        if (radius <= 0)
        {
            put_pixel(fb, cx, cy, color_swapped);
            return;
        }

        int x = radius;
        int y = 0;
        int err = 1 - x;

        while (x >= y)
        {
            if (filled)
            {
                for (int sx = cx - x; sx <= cx + x; sx++)
                {
                    put_pixel(fb, sx, cy + y, color_swapped);
                    put_pixel(fb, sx, cy - y, color_swapped);
                }
                for (int sx = cx - y; sx <= cx + y; sx++)
                {
                    put_pixel(fb, sx, cy + x, color_swapped);
                    put_pixel(fb, sx, cy - x, color_swapped);
                }
            }
            else
            {
                put_pixel(fb, cx + x, cy + y, color_swapped);
                put_pixel(fb, cx - x, cy + y, color_swapped);
                put_pixel(fb, cx + x, cy - y, color_swapped);
                put_pixel(fb, cx - x, cy - y, color_swapped);
                put_pixel(fb, cx + y, cy + x, color_swapped);
                put_pixel(fb, cx - y, cy + x, color_swapped);
                put_pixel(fb, cx + y, cy - x, color_swapped);
                put_pixel(fb, cx - y, cy - x, color_swapped);
            }

            y++;
            if (err < 0)
            {
                err += 2 * y + 1;
            }
            else
            {
                x--;
                err += 2 * (y - x) + 1;
            }
        }
    }

} // namespace blob_native
