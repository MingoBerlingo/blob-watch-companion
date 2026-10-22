#include "apps/blob_native/pages/page_manager.h"

#include <Arduino.h>

#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/pages/blob/blob_page.h"
#include "apps/blob_native/pages/brightness/brightness_page.h"
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
            Brightness,
        };

        Page g_page = Page::Blob;
        bool g_swipe_consumed = false;
        bool g_was_touching = false;

        void navigate(const WaveshareNativeTouchSample &touch)
        {
            // A gesture register can report the same swipe across many polls.
            // Rearm on a new contact or an idle sample after release.
            if ((touch.touching && !g_was_touching) ||
                (!touch.touching && touch.gesture == WaveshareNativeTouchGesture::None))
            {
                g_swipe_consumed = false;
            }
            g_was_touching = touch.touching;
            if (g_swipe_consumed ||
                (touch.gesture != WaveshareNativeTouchGesture::Left &&
                 touch.gesture != WaveshareNativeTouchGesture::Right))
            {
                return;
            }
            g_swipe_consumed = true;

            if (g_page == Page::Blob && touch.gesture == WaveshareNativeTouchGesture::Left)
            {
                timer_enter();
                blob_page_hide();
                timer_page_show();
                g_page = Page::Timer;
            }
            else if (g_page == Page::Timer && touch.gesture == WaveshareNativeTouchGesture::Right)
            {
                page_manager_show_blob();
            }
            else if (g_page == Page::Timer && touch.gesture == WaveshareNativeTouchGesture::Left)
            {
                timer_exit();
                timer_page_hide();
                brightness_page_show();
                g_page = Page::Brightness;
            }
            else if (g_page == Page::Brightness && touch.gesture == WaveshareNativeTouchGesture::Right)
            {
                brightness_page_hide();
                timer_enter();
                timer_page_show();
                g_page = Page::Timer;
            }
        }
    } // namespace

    void page_manager_show_blob()
    {
        if (g_page != Page::Timer)
        {
            return;
        }
        timer_exit();
        timer_page_hide();
        blob_page_show();
        g_page = Page::Blob;
    }

    void page_manager_init()
    {
        blob_page_init();
        timer_page_init();
        brightness_page_init();
        blob_page_show();
        g_page = Page::Blob;
        g_swipe_consumed = false;
        g_was_touching = false;
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
