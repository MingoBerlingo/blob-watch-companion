#include "apps/blob_native/timer/runtime_draw.h"

#include <math.h>

#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/shared_state.h"

namespace blob_native
{
    namespace
    {
        constexpr int kSegments = 48;
        constexpr int kButtonSize = 68;
        constexpr float kOuterRadius = 111.0f;
        constexpr float kInnerRadius = 95.0f;
        int g_lit_segments = -1;

        void line(lv_draw_ctx_t *context, lv_coord_t x0, lv_coord_t y0,
                  lv_coord_t x1, lv_coord_t y1, lv_color_t color, uint8_t width)
        {
            lv_draw_line_dsc_t descriptor;
            lv_draw_line_dsc_init(&descriptor);
            descriptor.color = color;
            descriptor.width = width;
            lv_point_t start = {x0, y0}, end = {x1, y1};
            lv_draw_line(context, &descriptor, &start, &end);
        }

        void draw_icon(lv_event_t *event)
        {
            lv_draw_ctx_t *context = lv_event_get_draw_ctx(event);
            if (!context) return;
            lv_area_t area;
            lv_obj_get_coords(lv_event_get_target(event), &area);
            const lv_coord_t x = (area.x1 + area.x2) / 2;
            const lv_coord_t y = (area.y1 + area.y2) / 2;
            const auto icon = static_cast<TimerIcon>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
            const lv_color_t black = lv_rgb565(BG_COLOR);
            if (icon == TimerIcon::Pause)
            {
                lv_draw_rect_dsc_t rectangle;
                lv_draw_rect_dsc_init(&rectangle);
                rectangle.bg_color = black;
                rectangle.bg_opa = LV_OPA_COVER;
                lv_area_t left = {static_cast<lv_coord_t>(x - 11), static_cast<lv_coord_t>(y - 14),
                                  static_cast<lv_coord_t>(x - 4), static_cast<lv_coord_t>(y + 14)};
                lv_area_t right = {static_cast<lv_coord_t>(x + 4), static_cast<lv_coord_t>(y - 14),
                                   static_cast<lv_coord_t>(x + 11), static_cast<lv_coord_t>(y + 14)};
                lv_draw_rect(context, &rectangle, &left);
                lv_draw_rect(context, &rectangle, &right);
            }
            else if (icon == TimerIcon::Play)
            {
                lv_draw_rect_dsc_t triangle;
                lv_draw_rect_dsc_init(&triangle);
                triangle.bg_color = black;
                triangle.bg_opa = LV_OPA_COVER;
                lv_point_t points[3] = {
                    {static_cast<lv_coord_t>(x - 8), static_cast<lv_coord_t>(y - 14)},
                    {static_cast<lv_coord_t>(x + 13), y},
                    {static_cast<lv_coord_t>(x - 8), static_cast<lv_coord_t>(y + 14)}};
                lv_draw_polygon(context, &triangle, points, 3);
            }
            else if (icon == TimerIcon::Back)
            {
                line(context, x + 7, y - 15, x - 8, y, black, 7);
                line(context, x - 8, y, x + 7, y + 15, black, 7);
            }
            else if (icon == TimerIcon::Repeat)
            {
                lv_draw_arc_dsc_t arc;
                lv_draw_arc_dsc_init(&arc);
                arc.color = black;
                arc.width = 7;
                lv_point_t center = {x, y};
                lv_draw_arc(context, &arc, &center, 18, 0, 300);
                lv_draw_rect_dsc_t arrow;
                lv_draw_rect_dsc_init(&arrow);
                arrow.bg_color = black;
                arrow.bg_opa = LV_OPA_COVER;
                lv_point_t points[3] = {
                    {static_cast<lv_coord_t>(x + 18), static_cast<lv_coord_t>(y - 8)},
                    {static_cast<lv_coord_t>(x + 6), static_cast<lv_coord_t>(y - 24)},
                    {static_cast<lv_coord_t>(x - 2), static_cast<lv_coord_t>(y - 8)}};
                lv_draw_polygon(context, &arrow, points, 3);
            }
            else
            {
                line(context, x - 10, y - 10, x + 10, y + 10, black, 7);
                line(context, x + 10, y - 10, x - 10, y + 10, black, 7);
            }
        }

        void draw_ring(lv_event_t *event)
        {
            lv_draw_ctx_t *context = lv_event_get_draw_ctx(event);
            if (!context) return;
            lv_area_t area;
            lv_obj_get_coords(lv_event_get_target(event), &area);
            const float center_x = area.x1 + CENTER_X;
            const float center_y = area.y1 + CENTER_Y;
            const lv_color_t color = lv_theme_get_color_primary(lv_event_get_target(event));
            for (int i = 0; i < g_lit_segments; ++i)
            {
                const float angle = (i * 360.0f / kSegments - 90.0f) * 0.01745329252f;
                const float dx = cosf(angle), dy = sinf(angle);
                line(context,
                     static_cast<lv_coord_t>(lroundf(center_x + kInnerRadius * dx)),
                     static_cast<lv_coord_t>(lroundf(center_y + kInnerRadius * dy)),
                     static_cast<lv_coord_t>(lroundf(center_x + kOuterRadius * dx)),
                     static_cast<lv_coord_t>(lroundf(center_y + kOuterRadius * dy)), color, 6);
            }
        }
    }

    lv_obj_t *timer_create_icon_button(lv_obj_t *parent, TimerIcon icon, uint16_t color,
                                       int16_t center_x, int16_t center_y)
    {
        lv_obj_t *button = lv_btn_create(parent);
        // Runtime controls are circular, independent of the setup button theme.
        lv_obj_remove_style_all(button);
        lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(button, kButtonSize, kButtonSize);
        lv_obj_set_pos(button, center_x - kButtonSize / 2, center_y - kButtonSize / 2);
        lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(button, lv_rgb565(color), 0);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(button, lv_color_darken(lv_rgb565(color), LV_OPA_30), LV_STATE_PRESSED);
        lv_obj_add_event_cb(button, draw_icon, LV_EVENT_DRAW_MAIN,
                            reinterpret_cast<void *>(static_cast<uintptr_t>(icon)));
        return button;
    }

    lv_obj_t *timer_create_progress_ring(lv_obj_t *parent)
    {
        lv_obj_t *ring = lv_obj_create(parent);
        lv_obj_remove_style_all(ring);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(ring, SCREEN_W, SCREEN_H);
        lv_obj_set_pos(ring, 0, 0);
        g_lit_segments = -1;
        lv_obj_add_event_cb(ring, draw_ring, LV_EVENT_DRAW_MAIN, nullptr);
        return ring;
    }

    void timer_refresh_progress_ring(lv_obj_t *ring, uint32_t remaining, uint32_t total)
    {
        int lit = total ? static_cast<int>((static_cast<uint64_t>(remaining) * kSegments + total - 1) / total) : 0;
        if (lit > kSegments) lit = kSegments;
        if (lit != g_lit_segments)
        {
            g_lit_segments = lit;
            lv_obj_invalidate(ring);
        }
    }
}
