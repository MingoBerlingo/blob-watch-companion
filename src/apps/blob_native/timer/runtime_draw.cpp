#include "apps/blob_native/timer/runtime_draw.h"

#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/shared_state.h"

namespace blob_native
{
    namespace
    {
        constexpr int kButtonSize = 68;

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
            if (!context)
                return;
            lv_area_t area;
            lv_obj_get_coords(lv_event_get_target(event), &area);
            const lv_coord_t x = area.x1 + lv_area_get_width(&area) / 2;
            const lv_coord_t y = area.y1 + lv_area_get_height(&area) / 2;
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
                // A filled outline gives both the outer tip and inner corner a true
                // miter. Two independently stroked lines leave a broken elbow.
                lv_draw_rect_dsc_t shape;
                lv_draw_rect_dsc_init(&shape);
                shape.bg_color = black;
                shape.bg_opa = LV_OPA_COVER;
                lv_point_t outline[5] = {
                    {static_cast<lv_coord_t>(x - 14), y},
                    {static_cast<lv_coord_t>(x + 5), static_cast<lv_coord_t>(y - 19)},
                    {static_cast<lv_coord_t>(x + 11), static_cast<lv_coord_t>(y - 13)},
                    {static_cast<lv_coord_t>(x + 11), static_cast<lv_coord_t>(y + 13)},
                    {static_cast<lv_coord_t>(x + 5), static_cast<lv_coord_t>(y + 19)}};
                lv_draw_polygon(context, &shape, outline, 5);
                // LVGL supports convex polygons. Subtract the notch using the
                // button's current color, including its pressed-state shade.
                shape.bg_color = lv_obj_get_style_bg_color(lv_event_get_target(event), LV_PART_MAIN);
                lv_point_t notch[3] = {
                    {static_cast<lv_coord_t>(x - 2), y},
                    {static_cast<lv_coord_t>(x + 14), static_cast<lv_coord_t>(y - 16)},
                    {static_cast<lv_coord_t>(x + 14), static_cast<lv_coord_t>(y + 16)}};
                lv_draw_polygon(context, &shape, notch, 3);
            }
            else if (icon == TimerIcon::Repeat)
            {
                lv_draw_arc_dsc_t arc;
                lv_draw_arc_dsc_init(&arc);
                arc.color = black;
                arc.width = 9;
                // Keep the circle centered on the button.
                lv_point_t center = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)};
                lv_draw_arc(context, &arc, &center, 19, 30, 300);
                lv_draw_rect_dsc_t arrow;
                lv_draw_rect_dsc_init(&arrow);
                arrow.bg_color = black;
                arrow.bg_opa = LV_OPA_COVER;
                // Clockwise tangent at the upper-right arc end. The broad base
                // overlaps the stroke while the tip leaves a clear right-side gap.
                lv_point_t points[3] = {
                    {static_cast<lv_coord_t>(x + 17), static_cast<lv_coord_t>(y - 4)},
                    {static_cast<lv_coord_t>(x + 0), static_cast<lv_coord_t>(y - 4)},
                    {static_cast<lv_coord_t>(x + 14), static_cast<lv_coord_t>(y - 22)}};
                lv_draw_polygon(context, &arrow, points, 3);
            }
            else
            {
                line(context, x - 10, y - 10, x + 10, y + 10, black, 7);
                line(context, x + 10, y - 10, x - 10, y + 10, black, 7);
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
        lv_obj_t *ring = lv_arc_create(parent);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(ring, 2, 2);
        lv_obj_set_size(ring, 236, 236);
        lv_obj_set_style_pad_all(ring, 0, 0);
        lv_arc_set_bg_angles(ring, 0, 360);
        lv_arc_set_rotation(ring, 270);
        lv_arc_set_range(ring, 0, 10000);
        lv_arc_set_value(ring, 0);
        lv_arc_set_mode(ring, LV_ARC_MODE_NORMAL);
        lv_obj_set_style_arc_width(ring, 8, LV_PART_MAIN);
        lv_obj_set_style_arc_width(ring, 8, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(ring, lv_rgb565(BG_COLOR), LV_PART_MAIN);
        lv_obj_set_style_arc_color(ring, lv_theme_get_color_primary(parent), LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(ring, true, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
        lv_obj_remove_style(ring, nullptr, LV_PART_KNOB);
        return ring;
    }

    void timer_refresh_progress_ring(lv_obj_t *ring, uint32_t remaining, uint32_t total)
    {
        const uint32_t bounded = remaining > total ? total : remaining;
        const int32_t value = total ? static_cast<int32_t>(static_cast<uint64_t>(bounded) * 10000 / total) : 0;
        lv_arc_set_value(ring, value);
    }
}
