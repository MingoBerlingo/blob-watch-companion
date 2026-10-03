#include "apps/blob_native/lv_port/lv_port.h"

#include <Arduino.h>
#include <string.h>

#include "apps/blob_native/shared_state.h"
#include "platform/waveshare_native_board.h"

namespace blob_native
{

    namespace
    {
        constexpr uint32_t DISP_BUF_PIXELS = (uint32_t)SCREEN_W * 20u;

        lv_disp_draw_buf_t g_draw_buf;
        lv_color_t g_draw_buf_1[DISP_BUF_PIXELS];
        lv_disp_drv_t g_disp_drv;
        lv_indev_drv_t g_indev_drv;

        void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
        {
            uint16_t *fb = waveshare_native_framebuffer();
            if (fb == nullptr)
            {
                lv_disp_flush_ready(drv);
                return;
            }

            // LV_COLOR_16_SWAP already stores the pixels in the panel's
            // byte-swapped RGB565 order, so a straight row copy is correct.
            const uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
            for (int y = area->y1; y <= area->y2; y++)
            {
                memcpy(&fb[y * SCREEN_W + area->x1],
                       &color_p[(y - area->y1) * w],
                       (size_t)w * sizeof(uint16_t));
            }

            waveshare_native_present_window(area->x1, area->y1, area->x2, area->y2);
            lv_disp_flush_ready(drv);
        }

        void input_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
        {
            (void)drv;
            WaveshareNativeTouchSample sample = {false, 0, 0, WaveshareNativeTouchGesture::None};
            if (waveshare_native_poll_touch(&sample) && sample.touching)
            {
                data->state = LV_INDEV_STATE_PRESSED;
                data->point.x = (lv_coord_t)sample.x;
                data->point.y = (lv_coord_t)sample.y;
            }
            else
            {
                data->state = LV_INDEV_STATE_RELEASED;
            }
        }

    } // namespace

    void lv_port_init()
    {
        lv_init();

        lv_disp_draw_buf_init(&g_draw_buf, g_draw_buf_1, nullptr, DISP_BUF_PIXELS);

        lv_disp_drv_init(&g_disp_drv);
        g_disp_drv.hor_res = SCREEN_W;
        g_disp_drv.ver_res = SCREEN_H;
        g_disp_drv.flush_cb = flush_cb;
        g_disp_drv.draw_buf = &g_draw_buf;
        lv_disp_drv_register(&g_disp_drv);

        lv_indev_drv_init(&g_indev_drv);
        g_indev_drv.type = LV_INDEV_TYPE_POINTER;
        g_indev_drv.read_cb = input_read_cb;
        lv_indev_drv_register(&g_indev_drv);
    }

    void lv_port_handler()
    {
        lv_timer_handler();
    }

    lv_color_t lv_rgb565(uint16_t color)
    {
        const uint8_t r5 = (uint8_t)((color >> 11) & 0x1Fu);
        const uint8_t g6 = (uint8_t)((color >> 5) & 0x3Fu);
        const uint8_t b5 = (uint8_t)(color & 0x1Fu);
        return lv_color_make((uint8_t)((r5 << 3) | (r5 >> 2)),
                             (uint8_t)((g6 << 2) | (g6 >> 4)),
                             (uint8_t)((b5 << 3) | (b5 >> 2)));
    }

} // namespace blob_native
