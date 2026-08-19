#include "apps/blob_native/ui/button.h"

#include <math.h>

#include "GUI_Paint.h"
#include "apps/blob_native/ui/draw.h"

namespace blob_native
{
    namespace ui
    {
        bool button_contains(const RectButton &button, uint16_t x, uint16_t y)
        {
            return ((int)x >= button.x0 && (int)x <= button.x1 &&
                    (int)y >= button.y0 && (int)y <= button.y1);
        }

        bool button_contains(const CircleButton &button, uint16_t x, uint16_t y)
        {
            const int32_t dx = (int32_t)x - button.cx;
            const int32_t dy = (int32_t)y - button.cy;
            return (dx * dx + dy * dy) <= (int32_t)button.radius * (int32_t)button.radius;
        }

        void draw_button(const RectButton &button)
        {
            Paint_DrawRectangle(button.x0, button.y0, button.x1, button.y1,
                                button.filled ? button.fill_color : button.border_color,
                                DOT_PIXEL_1X1,
                                button.filled ? DRAW_FILL_FULL : DRAW_FILL_EMPTY);

            if (button.filled && button.border_color != button.fill_color)
            {
                Paint_DrawRectangle(button.x0, button.y0, button.x1, button.y1,
                                    button.border_color, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
            }
        }

        void draw_button(const CircleButton &button)
        {
            draw_circle(button.cx, button.cy, button.radius,
                        button.filled ? button.fill_color : button.border_color,
                        button.filled);

            if (button.filled && button.border_color != button.fill_color)
            {
                draw_circle(button.cx, button.cy, button.radius,
                            button.border_color, false);
            }
        }
    } // namespace ui
} // namespace blob_native