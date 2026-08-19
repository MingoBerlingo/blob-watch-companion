#ifndef BLOB_NATIVE_UI_BUTTON_H
#define BLOB_NATIVE_UI_BUTTON_H

#include <stdint.h>

namespace blob_native
{
    namespace ui
    {
        struct RectButton
        {
            int16_t x0;
            int16_t y0;
            int16_t x1;
            int16_t y1;
            uint16_t fill_color;
            uint16_t border_color;
            bool filled;
        };

        struct CircleButton
        {
            int16_t cx;
            int16_t cy;
            int16_t radius;
            uint16_t fill_color;
            uint16_t border_color;
            bool filled;
        };

        bool button_contains(const RectButton &button, uint16_t x, uint16_t y);
        bool button_contains(const CircleButton &button, uint16_t x, uint16_t y);

        void draw_button(const RectButton &button);
        void draw_button(const CircleButton &button);
    } // namespace ui
} // namespace blob_native

#endif