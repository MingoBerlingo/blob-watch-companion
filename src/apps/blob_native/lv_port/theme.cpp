#include "apps/blob_native/lv_port/theme.h"

#include "lvgl.h"

#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/shared_state.h"

namespace blob_native
{

    namespace
    {
        // Theme palette (native RGB565 values, converted via lv_rgb565).
        constexpr uint16_t kThemePrimary = 0x781F;   // main accent (violet)
        constexpr uint16_t kThemeSecondary = 0x7FFF; // secondary accent (bright cyan)

        // Shared component styles, applied automatically by the custom theme.
        static lv_style_t style_roller_main;
        static lv_style_t style_roller_selected;
        static lv_style_t style_btn;

        void apply_cb(lv_theme_t *th, lv_obj_t *obj)
        {
            (void)th;
            if (lv_obj_check_type(obj, &lv_roller_class))
            {
                lv_obj_add_style(obj, &style_roller_main, LV_PART_MAIN);
                lv_obj_add_style(obj, &style_roller_selected, LV_PART_SELECTED);
            }
            else if (lv_obj_check_type(obj, &lv_btn_class))
            {
                lv_obj_add_style(obj, &style_btn, 0);
            }
        }
    } // namespace

    void lv_port_apply_theme()
    {
        // Roller: main (unselected) options.
        lv_style_init(&style_roller_main);
        lv_style_set_radius(&style_roller_main, 20);
        lv_style_set_border_width(&style_roller_main, 0);
        lv_style_set_text_opa(&style_roller_main, (255 * 30 / 100));
        lv_style_set_bg_opa(&style_roller_main, (255 * 50 / 100));
        lv_style_set_text_line_space(&style_roller_main, 4);
        lv_style_set_text_font(&style_roller_main, &lv_font_montserrat_24);

        // Roller: selected option.
        lv_style_init(&style_roller_selected);
        lv_style_set_text_opa(&style_roller_selected, (255 * 100 / 100));
        lv_style_set_bg_opa(&style_roller_selected, (255 * 0 / 100));

        // Button: size, shape, and font (colors come from the base theme).
        lv_style_init(&style_btn);
        lv_style_set_radius(&style_btn, 20);
        lv_style_set_text_font(&style_btn, &lv_font_montserrat_24);
        lv_style_set_width(&style_btn, 100);
        lv_style_set_height(&style_btn, 60);

        lv_theme_t *base = lv_theme_default_init(nullptr,
                                                 lv_rgb565(kThemePrimary),
                                                 lv_rgb565(kThemeSecondary),
                                                 true,
                                                 &lv_font_montserrat_14);

        static lv_theme_t th_new;
        th_new = *base;
        lv_theme_set_parent(&th_new, base);
        lv_theme_set_apply_cb(&th_new, apply_cb);
        lv_disp_set_theme(nullptr, &th_new);
    }

} // namespace blob_native
