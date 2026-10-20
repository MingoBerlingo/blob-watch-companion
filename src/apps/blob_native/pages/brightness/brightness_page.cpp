#include "apps/blob_native/pages/brightness/brightness_page.h"

#include "lvgl.h"
#include "platform/waveshare_native_board.h"

namespace blob_native {
namespace {
lv_obj_t *g_screen = nullptr;
lv_obj_t *g_slider = nullptr;
lv_obj_t *g_value = nullptr;
uint8_t g_brightness = 70;

void on_changed(lv_event_t *event) {
    const int32_t value = lv_slider_get_value(lv_event_get_target(event));
    g_brightness = static_cast<uint8_t>(value);
    waveshare_native_set_brightness(g_brightness);
    lv_label_set_text_fmt(g_value, "%d%%", static_cast<int>(g_brightness));
}

void add_label(const char *text, int16_t y, const lv_font_t *font, lv_color_t color) {
    lv_obj_t *label = lv_label_create(g_screen);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, y);
}
}

void brightness_page_init() {
    g_screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(g_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(g_screen, 0, 0);
    lv_obj_set_style_pad_all(g_screen, 0, 0);

    add_label("Brightness", 34, &lv_font_montserrat_20, lv_color_white());
    add_label("Small sun", 89, &lv_font_montserrat_12, lv_color_hex(0x8E8E93));
    add_label("Large sun", 157, &lv_font_montserrat_12, lv_color_hex(0x8E8E93));

    // iOS-inspired dark appearance: a wide, rounded, white fill on a dark track.
    g_slider = lv_slider_create(g_screen);
    lv_obj_set_size(g_slider, 180, 34);
    lv_obj_align(g_slider, LV_ALIGN_TOP_MID, 0, 115);
    lv_slider_set_range(g_slider, 5, 100);
    lv_slider_set_value(g_slider, g_brightness, LV_ANIM_OFF);
    lv_obj_set_style_radius(g_slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_radius(g_slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(g_slider, lv_color_hex(0x39393D), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(g_slider, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(g_slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(g_slider, 0, LV_PART_KNOB);
    lv_obj_add_event_cb(g_slider, on_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    g_value = lv_label_create(g_screen);
    lv_obj_set_style_text_font(g_value, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(g_value, lv_color_hex(0xAEAEB2), 0);
    lv_obj_align(g_value, LV_ALIGN_TOP_MID, 0, 189);
    lv_label_set_text_fmt(g_value, "%d%%", static_cast<int>(g_brightness));
}

void brightness_page_show() {
    lv_scr_load(g_screen);
    lv_slider_set_value(g_slider, g_brightness, LV_ANIM_OFF);
}
void brightness_page_hide() {}
}
