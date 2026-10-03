#include "apps/blob_native/pages/page_manager.h"

#include <Arduino.h>

#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/pages/blob/blob_page.h"
#include "apps/blob_native/pages/timer/timer_page.h"
#include "apps/blob_native/timer/view.h"
#include "platform/waveshare_native_board.h"

namespace blob_native
{

    namespace
    {
        enum class Page : uint8_t
        {
            Blob = 0,
            Timer,
        };

        Page g_page = Page::Blob;

        void navigate(const WaveshareNativeTouchSample &touch)
        {
            if (g_page == Page::Blob)
            {
                if (touch.gesture == WaveshareNativeTouchGesture::Left)
                {
                    timer_enter();
                    blob_page_hide();
                    timer_page_show();
                    g_page = Page::Timer;
                }
                return;
            }

            if (touch.gesture == WaveshareNativeTouchGesture::Right)
            {
                timer_exit();
                timer_page_hide();
                blob_page_show();
                g_page = Page::Blob;
            }
        }
    } // namespace

    void page_manager_init()
    {
        blob_page_init();
        timer_page_init();
        blob_page_show();
        g_page = Page::Blob;
    }

    void page_manager_loop()
    {
        const uint32_t now_ms = millis();

        WaveshareNativeTouchSample touch = {false, 0, 0, WaveshareNativeTouchGesture::None};
        waveshare_native_poll_touch(&touch);

        timer_update_remaining(now_ms);
        navigate(touch);

        if (g_page == Page::Timer)
        {
            timer_page_tick(now_ms);
        }

        lv_port_handler();
        delay(5);
    }

} // namespace blob_native
