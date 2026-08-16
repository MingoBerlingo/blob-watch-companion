#include "apps/blob_native/blob/overlay.h"

#include "apps/blob_native/shared_state.h"

namespace blob_native
{

    namespace
    {

        constexpr int16_t kOverlayScale = 1;
        constexpr int16_t kOverlayW = 80;
        constexpr int16_t kOverlayH = 16;

        static inline uint16_t to_panel_rgb565(uint16_t color)
        {
            return (uint16_t)((color << 8) | (color >> 8));
        }

        static inline void fb_put_pixel(uint16_t *fb, int x, int y, uint16_t color_swapped)
        {
            if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H)
            {
                return;
            }
            fb[y * SCREEN_W + x] = color_swapped;
        }

        static inline void fb_put_block(uint16_t *fb, int x, int y, int scale, uint16_t color_swapped)
        {
            for (int dy = 0; dy < scale; dy++)
            {
                for (int dx = 0; dx < scale; dx++)
                {
                    fb_put_pixel(fb, x + dx, y + dy, color_swapped);
                }
            }
        }

        static void draw_3x5_glyph(uint16_t *fb, int x, int y, const uint8_t glyph[5], int scale, uint16_t color_swapped)
        {
            for (int row = 0; row < 5; row++)
            {
                const uint8_t bits = glyph[row];
                for (int col = 0; col < 3; col++)
                {
                    if (bits & (1u << (2 - col)))
                    {
                        fb_put_block(fb, x + col * scale, y + row * scale, scale, color_swapped);
                    }
                }
            }
        }

        static void draw_3x5_digit(uint16_t *fb, int x, int y, int digit, int scale, uint16_t color_swapped)
        {
            static const uint8_t glyphs[10][5] = {
                {0b111, 0b101, 0b101, 0b101, 0b111},
                {0b010, 0b110, 0b010, 0b010, 0b111},
                {0b111, 0b001, 0b111, 0b100, 0b111},
                {0b111, 0b001, 0b111, 0b001, 0b111},
                {0b101, 0b101, 0b111, 0b001, 0b001},
                {0b111, 0b100, 0b111, 0b001, 0b111},
                {0b111, 0b100, 0b111, 0b101, 0b111},
                {0b111, 0b001, 0b001, 0b001, 0b001},
                {0b111, 0b101, 0b111, 0b101, 0b111},
                {0b111, 0b101, 0b111, 0b001, 0b111},
            };

            if (digit < 0 || digit > 9)
            {
                return;
            }

            draw_3x5_glyph(fb, x, y, glyphs[digit], scale, color_swapped);
        }

        static void draw_3x5_char(uint16_t *fb, int x, int y, char c, int scale, uint16_t color_swapped)
        {
            static const uint8_t glyph_f[5] = {0b111, 0b100, 0b111, 0b100, 0b100};
            static const uint8_t glyph_p[5] = {0b110, 0b101, 0b110, 0b100, 0b100};
            static const uint8_t glyph_s[5] = {0b111, 0b100, 0b111, 0b001, 0b111};
            static const uint8_t glyph_w[5] = {0b101, 0b101, 0b101, 0b111, 0b101};
            static const uint8_t glyph_h[5] = {0b101, 0b101, 0b111, 0b101, 0b101};
            static const uint8_t glyph_x[5] = {0b101, 0b101, 0b010, 0b101, 0b101};

            if (c >= '0' && c <= '9')
            {
                draw_3x5_digit(fb, x, y, c - '0', scale, color_swapped);
                return;
            }

            if (c == 'F')
                draw_3x5_glyph(fb, x, y, glyph_f, scale, color_swapped);
            else if (c == 'P')
                draw_3x5_glyph(fb, x, y, glyph_p, scale, color_swapped);
            else if (c == 'S')
                draw_3x5_glyph(fb, x, y, glyph_s, scale, color_swapped);
            else if (c == 'W')
                draw_3x5_glyph(fb, x, y, glyph_w, scale, color_swapped);
            else if (c == 'H')
                draw_3x5_glyph(fb, x, y, glyph_h, scale, color_swapped);
            else if (c == 'x')
                draw_3x5_glyph(fb, x, y, glyph_x, scale, color_swapped);
        }

        static void draw_3x5_text(uint16_t *fb, int x, int y, const char *text, int scale, uint16_t color_swapped)
        {
            if (text == nullptr)
            {
                return;
            }

            int cursor_x = x;
            for (const char *p = text; *p != '\0'; p++)
            {
                if (*p != ' ')
                {
                    draw_3x5_char(fb, cursor_x, y, *p, scale, color_swapped);
                }
                cursor_x += 4 * scale;
            }
        }

        static inline void append_char(char *dst, int &idx, int max_len, char c)
        {
            if (idx < (max_len - 1))
            {
                dst[idx++] = c;
            }
        }

        static void append_uint(char *dst, int &idx, int max_len, uint32_t value)
        {
            char tmp[10];
            int count = 0;
            do
            {
                tmp[count++] = (char)('0' + (value % 10));
                value /= 10;
            } while (value != 0 && count < (int)sizeof(tmp));

            for (int i = count - 1; i >= 0; i--)
            {
                append_char(dst, idx, max_len, tmp[i]);
            }
        }

    } // namespace

    void overlay_expand_dirty_bounds(int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
    {
        if (min_x == nullptr || min_y == nullptr || max_x == nullptr || max_y == nullptr)
        {
            return;
        }

        const int16_t ox0 = clamp_i16(BLOB_PERF_OVERLAY_X - 1, 0, SCREEN_W - 1);
        const int16_t oy0 = clamp_i16(BLOB_PERF_OVERLAY_Y - 1, 0, SCREEN_H - 1);
        const int16_t ox1 = clamp_i16(BLOB_PERF_OVERLAY_X + kOverlayW, 0, SCREEN_W - 1);
        const int16_t oy1 = clamp_i16(BLOB_PERF_OVERLAY_Y + kOverlayH, 0, SCREEN_H - 1);

        if (ox0 < *min_x)
            *min_x = ox0;
        if (oy0 < *min_y)
            *min_y = oy0;
        if (ox1 > *max_x)
            *max_x = ox1;
        if (oy1 > *max_y)
            *max_y = oy1;
    }

    void overlay_draw(uint16_t *fb, uint16_t fps, int dirty_w, int dirty_h)
    {
        if (fb == nullptr)
        {
            return;
        }

        const uint16_t text = to_panel_rgb565(CYAN);

        char line1[22] = {0};
        int idx1 = 0;
        append_char(line1, idx1, (int)sizeof(line1), 'F');
        append_char(line1, idx1, (int)sizeof(line1), 'P');
        append_char(line1, idx1, (int)sizeof(line1), 'S');
        append_char(line1, idx1, (int)sizeof(line1), ' ');
        append_uint(line1, idx1, (int)sizeof(line1), fps);

        char line2[22] = {0};
        int idx2 = 0;
        append_char(line2, idx2, (int)sizeof(line2), 'W');
        append_char(line2, idx2, (int)sizeof(line2), ' ');
        append_uint(line2, idx2, (int)sizeof(line2), (uint32_t)dirty_w);
        append_char(line2, idx2, (int)sizeof(line2), 'x');
        append_char(line2, idx2, (int)sizeof(line2), 'H');
        append_char(line2, idx2, (int)sizeof(line2), ' ');
        append_uint(line2, idx2, (int)sizeof(line2), (uint32_t)dirty_h);

        draw_3x5_text(fb, BLOB_PERF_OVERLAY_X, BLOB_PERF_OVERLAY_Y, line1, kOverlayScale, text);
        draw_3x5_text(fb, BLOB_PERF_OVERLAY_X, BLOB_PERF_OVERLAY_Y + 7, line2, kOverlayScale, text);
    }

} // namespace blob_native
