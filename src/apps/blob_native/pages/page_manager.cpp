#include "apps/blob_native/pages/page_manager.h"

#include <Arduino.h>
#include "apps/blob_native/pages/steps/steps_page.h"
#include "apps/blob_native/steps/pedometer.h"
#include "apps/blob_native/blob/physics_settings.h"
#include "apps/blob_native/pages/settings/settings_page.h"

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
            Settings,
            Steps,
        };

        Page g_page = Page::Blob;
        bool g_swipe_consumed = false;
        bool g_was_touching = false;
        uint32_t g_press_ms=0;
        int g_press_x=0,g_press_y=0;
        bool g_press_moved=false;

        void navigate(const WaveshareNativeTouchSample &touch)
        {
            // A gesture register can report the same swipe across many polls.
            // Rearm on a new contact or an idle sample after release.
            if ((touch.touching && !g_was_touching) ||
                (!touch.touching && touch.gesture == WaveshareNativeTouchGesture::None))
            {
                g_swipe_consumed = false;
                if(touch.touching) {
                    g_press_ms=millis();g_press_x=touch.x;g_press_y=touch.y;g_press_moved=false;
                }
            }
            g_was_touching = touch.touching;
            const int dx=static_cast<int>(touch.x)-g_press_x,dy=static_cast<int>(touch.y)-g_press_y;
            if(touch.touching && dx*dx+dy*dy>144) g_press_moved=true;
            // Settings owns its sliders: horizontal drags must not navigate.
            if(g_page==Page::Settings || (g_page==Page::Steps && steps_page_reset_open())) return;
            if(g_page==Page::Steps && touch.touching && !g_swipe_consumed && !g_press_moved &&
               (millis()-g_press_ms>=650 || touch.gesture==WaveshareNativeTouchGesture::LongPress)) {
                g_swipe_consumed=true;lv_port_block_touch_until_release();steps_page_request_reset();return;
            }
            if(g_page==Page::Blob && touch.touching && !g_swipe_consumed && !g_press_moved &&
               (millis()-g_press_ms>=650 || touch.gesture==WaveshareNativeTouchGesture::LongPress)) {
                g_swipe_consumed=true;
                lv_port_block_touch_until_release();
                blob_page_hide();settings_page_show();g_page=Page::Settings;
                return;
            }
            if (g_swipe_consumed ||
                (touch.gesture != WaveshareNativeTouchGesture::Left &&
                 touch.gesture != WaveshareNativeTouchGesture::Right))
            {
                return;
            }
            g_swipe_consumed = true;
            lv_port_block_touch_until_release();

            if(g_page==Page::Blob && touch.gesture==WaveshareNativeTouchGesture::Right) {
                blob_page_hide();steps_page_show();g_page=Page::Steps;
            }
            else if(g_page==Page::Steps && touch.gesture==WaveshareNativeTouchGesture::Left) {
                blob_page_show();g_page=Page::Blob;
            }
            else if (g_page == Page::Blob && touch.gesture == WaveshareNativeTouchGesture::Left)
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
        if(g_page==Page::Settings) {
            lv_port_block_touch_until_release();
            blob_page_show();g_page=Page::Blob;
            g_swipe_consumed=true;
            return;
        }
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
        pedometer_init(millis());
        steps_page_init();
        physics_settings_reset();
        physics_settings_load();
        settings_page_init();
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

        pedometer_tick(now_ms);
        timer_update_remaining(now_ms);
        navigate(touch);

        if (g_page == Page::Timer)
        {
            timer_page_tick(now_ms);
        }

        if(g_page==Page::Steps) steps_page_tick();

        lv_port_handler();
        delay(5);
    }

} // namespace blob_native
