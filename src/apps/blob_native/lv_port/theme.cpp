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
    } // namespace

    void lv_port_apply_theme()
    {
        lv_theme_default_init(nullptr,
                              lv_rgb565(kThemePrimary),
                              lv_rgb565(kThemeSecondary),
                              true,
                              &lv_font_montserrat_14);
    }

} // namespace blob_native
