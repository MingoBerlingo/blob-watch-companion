#ifndef BLOB_NATIVE_LV_PORT_H
#define BLOB_NATIVE_LV_PORT_H

#include <stdint.h>

#include "lvgl.h"

namespace blob_native
{

    void lv_port_init();
    void lv_port_handler();

    // Convert a native RGB565 color (as used elsewhere in this project, e.g.
    // the GUI_Paint.h constants) to an LVGL color, preserving the exact 5-6-5
    // round-trip. LV_COLOR_16_SWAP handles the panel byte order.
    lv_color_t lv_rgb565(uint16_t color);

} // namespace blob_native

#endif
