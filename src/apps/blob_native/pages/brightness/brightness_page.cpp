#include "apps/blob_native/pages/brightness/brightness_page.h"

#include "lvgl.h"
#include "platform/waveshare_native_board.h"

namespace blob_native {
namespace {
lv_obj_t *g_screen = nullptr;
lv_obj_t *g_slider = nullptr;
uint8_t g_brightness = 70;

void on_changed(lv_event_t *event) {
    const int32_t value = lv_slider_get_value(lv_event_get_target(event));
    g_brightness = static_cast<uint8_t>(value);
    waveshare_native_set_brightness(g_brightness);
}

void draw_sun(lv_event_t *event) {
    auto *context = lv_event_get_draw_ctx(event);
    lv_area_t area;
    lv_obj_get_coords(lv_event_get_target(event), &area);
    const lv_coord_t x = area.x1 + 14, y = area.y1 + 14;
    lv_draw_rect_dsc_t disk;
    lv_draw_rect_dsc_init(&disk);
    disk.bg_color = lv_color_white();
    disk.bg_opa = LV_OPA_COVER;
    disk.radius = LV_RADIUS_CIRCLE;
    lv_area_t center = {static_cast<lv_coord_t>(x - 5), static_cast<lv_coord_t>(y - 5),
                        static_cast<lv_coord_t>(x + 5), static_cast<lv_coord_t>(y + 5)};
    lv_draw_rect(context, &disk, &center);
    lv_draw_line_dsc_t ray;
    lv_draw_line_dsc_init(&ray);
    ray.color = lv_color_white();
    ray.width = 2;
    ray.round_start = ray.round_end = true;
    const int8_t ends[8][4] = {{0,-10,0,-13},{7,-7,9,-9},{10,0,13,0},{7,7,9,9},
                              {0,10,0,13},{-7,7,-9,9},{-10,0,-13,0},{-7,-7,-9,-9}};
    for (const auto &r : ends) {
        lv_point_t a = {static_cast<lv_coord_t>(x+r[0]), static_cast<lv_coord_t>(y+r[1])};
        lv_point_t b = {static_cast<lv_coord_t>(x+r[2]), static_cast<lv_coord_t>(y+r[3])};
        lv_draw_line(context, &ray, &a, &b);
    }
}
}

void brightness_page_init() {
    g_screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(g_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_screen, 0, 0);
    lv_obj_set_style_pad_all(g_screen, 0, 0);

    auto *sun = lv_obj_create(g_screen);
    lv_obj_remove_style_all(sun);
    lv_obj_clear_flag(sun, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(sun, 28, 28);
    lv_obj_align(sun, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_add_event_cb(sun, draw_sun, LV_EVENT_DRAW_MAIN, nullptr);

    // iOS-inspired dark appearance: a wide, rounded, white fill on a dark track.
    g_slider = lv_slider_create(g_screen);
    lv_obj_set_size(g_slider, 60, 150);
    lv_obj_align(g_slider, LV_ALIGN_TOP_MID, 0, 60);
    lv_slider_set_range(g_slider, 5, 100);
    lv_slider_set_value(g_slider, g_brightness, LV_ANIM_OFF);
    lv_obj_set_style_radius(g_slider, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(g_slider, 12, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(g_slider, lv_color_hex(0x39393D), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_slider, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(g_slider, 0, LV_PART_KNOB);
    lv_obj_add_event_cb(g_slider, on_changed, LV_EVENT_VALUE_CHANGED, nullptr);

}

void brightness_page_show() {
    lv_scr_load(g_screen);
    lv_slider_set_value(g_slider, g_brightness, LV_ANIM_OFF);
}
void brightness_page_hide() {}
}
