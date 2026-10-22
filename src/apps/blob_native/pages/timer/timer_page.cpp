#include "apps/blob_native/pages/timer/timer_page.h"

#include <Arduino.h>
#include <stdio.h>

#include "lvgl.h"

#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/shared_state.h"
#include "apps/blob_native/pages/page_manager.h"
#include "apps/blob_native/timer/runtime_draw.h"
#include "apps/blob_native/timer/view.h"

namespace blob_native
{

    namespace
    {
        constexpr int kRollerOptions = 60; // "00" .. "59"

        lv_obj_t *g_screen = nullptr;

        lv_obj_t *g_minutes_roller = nullptr;
        lv_obj_t *g_seconds_roller = nullptr;
        lv_obj_t *g_start_btn = nullptr;

        lv_obj_t *g_ring = nullptr;
        lv_obj_t *g_pause_btn = nullptr;
        lv_obj_t *g_time_label = nullptr;
        lv_obj_t *g_back_btn = nullptr;
        lv_obj_t *g_play_btn = nullptr;
        lv_obj_t *g_cancel_btn = nullptr;
        lv_obj_t *g_ended_label = nullptr;
        lv_obj_t *g_ended_back_btn = nullptr;
        lv_obj_t *g_repeat_btn = nullptr;

        // Roller options string ("00\n01\n...\n59").
        char g_roller_options[4 * kRollerOptions];

        // Change tracking.
        int32_t g_last_display_seconds = -1;
        bool g_last_controls = false;
        TimerView g_last_view = TimerView::MainScreen;

        lv_color_t col(uint16_t c)
        {
            return lv_rgb565(c);
        }

        void set_visible(lv_obj_t *obj, bool visible)
        {
            if (visible)
                lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }

        void build_roller_options()
        {
            int idx = 0;
            for (int i = 0; i < kRollerOptions; i++)
            {
                g_roller_options[idx++] = (char)('0' + (i / 10));
                g_roller_options[idx++] = (char)('0' + (i % 10));
                g_roller_options[idx++] = (i == kRollerOptions - 1) ? '\0' : '\n';
            }
        }

        void on_minutes_changed(lv_event_t *e)
        {
            lv_obj_t *roller = lv_event_get_target(e);
            timer_set_minutes((int)lv_roller_get_selected(roller));
        }

        void on_seconds_changed(lv_event_t *e)
        {
            lv_obj_t *roller = lv_event_get_target(e);
            timer_set_seconds((int)lv_roller_get_selected(roller));
        }

        void on_start_clicked(lv_event_t *e)
        {
            (void)e;
            timer_start(millis());
        }

        void on_back_clicked(lv_event_t *e)
        {
            (void)e;
            page_manager_show_blob();
        }

        void on_play_clicked(lv_event_t *e)
        {
            (void)e;
            timer_action(millis());
        }

        void on_cancel_clicked(lv_event_t *e)
        {
            (void)e;
            timer_cancel();
        }

        void build_screen()
        {
            build_roller_options();

            g_screen = lv_obj_create(nullptr);
            lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(g_screen, col(BG_COLOR), 0);
            lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);

            // --- Setup view ---
            g_minutes_roller = lv_roller_create(g_screen);
            lv_roller_set_options(g_minutes_roller, g_roller_options, LV_ROLLER_MODE_NORMAL);
            lv_roller_set_visible_row_count(g_minutes_roller, 3);
            lv_obj_set_pos(g_minutes_roller, 35, 70);
            lv_obj_set_size(g_minutes_roller, 80, 80);
            lv_obj_add_event_cb(g_minutes_roller, on_minutes_changed, LV_EVENT_VALUE_CHANGED, nullptr);

            g_seconds_roller = lv_roller_create(g_screen);
            lv_roller_set_options(g_seconds_roller, g_roller_options, LV_ROLLER_MODE_NORMAL);
            lv_roller_set_visible_row_count(g_seconds_roller, 3);
            lv_obj_set_pos(g_seconds_roller, 125, 70);
            lv_obj_set_size(g_seconds_roller, 80, 80);
            lv_obj_add_event_cb(g_seconds_roller, on_seconds_changed, LV_EVENT_VALUE_CHANGED, nullptr);

            g_start_btn = lv_btn_create(g_screen);
            lv_obj_set_pos(g_start_btn, 65, 170);
            lv_obj_add_event_cb(g_start_btn, on_start_clicked, LV_EVENT_CLICKED, nullptr);

            lv_obj_t *start_label = lv_label_create(g_start_btn);
            lv_label_set_text(start_label, "Start");
            lv_obj_center(start_label);

            // --- Run view: shared ring with distinct running and paused controls. ---
            g_ring = timer_create_progress_ring(g_screen);

            g_time_label = lv_label_create(g_screen);
            lv_obj_clear_flag(g_time_label, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_text_font(g_time_label, &lv_font_montserrat_32, 0);
            lv_obj_set_style_text_color(g_time_label, lv_color_white(), 0);

            g_pause_btn = timer_create_icon_button(g_screen, TimerIcon::Pause, TIMER_PAUSE_COLOR, 120, 164);
            g_back_btn = timer_create_icon_button(g_screen, TimerIcon::Back, TIMER_BACK_COLOR, 82, 98);
            g_cancel_btn = timer_create_icon_button(g_screen, TimerIcon::Cancel, TIMER_CANCEL_COLOR, 158, 98);
            g_play_btn = timer_create_icon_button(g_screen, TimerIcon::Play, TIMER_PLAY_COLOR, 120, 164);

            lv_obj_add_event_cb(g_pause_btn, on_play_clicked, LV_EVENT_CLICKED, nullptr);
            lv_obj_add_event_cb(g_back_btn, on_back_clicked, LV_EVENT_CLICKED, nullptr);
            lv_obj_add_event_cb(g_play_btn, on_play_clicked, LV_EVENT_CLICKED, nullptr);
            lv_obj_add_event_cb(g_cancel_btn, on_cancel_clicked, LV_EVENT_CLICKED, nullptr);

            // --- Ended view: duration, completion message, back to setup and repeat. ---
            g_ended_label = lv_label_create(g_screen);
            lv_label_set_text(g_ended_label, "Timer ended");
            lv_obj_set_style_text_font(g_ended_label, &lv_font_montserrat_20, 0);
            lv_obj_set_style_text_color(g_ended_label, lv_color_white(), 0);
            lv_obj_align(g_ended_label, LV_ALIGN_TOP_MID, 0, 66);
            g_ended_back_btn = timer_create_icon_button(g_screen, TimerIcon::Back, TIMER_BACK_COLOR, 82, 138);
            g_repeat_btn = timer_create_icon_button(g_screen, TimerIcon::Repeat, TIMER_PAUSE_COLOR, 158, 138);
            lv_obj_add_event_cb(g_ended_back_btn, on_cancel_clicked, LV_EVENT_CLICKED, nullptr);
            lv_obj_add_event_cb(g_repeat_btn, on_play_clicked, LV_EVENT_CLICKED, nullptr);
        }

        void sync(bool force)
        {
            const TimerView view = timer_view();
            const bool controls = timer_run_controls_visible();
            const bool show_setup = (view == TimerView::TimerSetup);
            const bool show_run = (view == TimerView::TimerRun);
            const bool show_ended = (view == TimerView::TimerEnded);

            set_visible(g_minutes_roller, show_setup);
            set_visible(g_seconds_roller, show_setup);
            set_visible(g_start_btn, show_setup);
            set_visible(g_ring, show_run);
            set_visible(g_time_label, show_run || show_ended);
            set_visible(g_pause_btn, show_run && !controls);
            set_visible(g_back_btn, show_run && controls);
            set_visible(g_cancel_btn, show_run && controls);
            set_visible(g_play_btn, show_run && controls);
            set_visible(g_ended_label, show_ended);
            set_visible(g_ended_back_btn, show_ended);
            set_visible(g_repeat_btn, show_ended);

            if (show_setup && force)
            {
                lv_roller_set_selected(g_minutes_roller, (uint16_t)timer_minutes(), LV_ANIM_OFF);
                lv_roller_set_selected(g_seconds_roller, (uint16_t)timer_seconds(), LV_ANIM_OFF);
            }

            if (show_run || show_ended)
            {
                const int32_t disp = show_ended ? timer_total_seconds() : timer_display_seconds_value();
                if (force || disp != g_last_display_seconds)
                {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%02ld:%02ld", (long)(disp / 60), (long)(disp % 60));
                    lv_label_set_text(g_time_label, buf);
                    g_last_display_seconds = disp;
                }

                if (show_run) timer_refresh_progress_ring(g_ring, timer_remaining_ms(), timer_total_ms());
                if (force || controls != g_last_controls || view != g_last_view)
                {
                    lv_obj_set_style_text_font(g_time_label, show_ended ? &lv_font_montserrat_16 : (controls ? &lv_font_montserrat_14 : &lv_font_montserrat_32), 0);
                    lv_obj_set_style_text_color(g_time_label, show_ended ? col(TIMER_ENDED_TIME_COLOR) : (controls ? col(TIMER_BACK_COLOR) : lv_color_white()), 0);
                    g_last_controls = controls;
                }
                // Re-align after text updates so changing digit widths stay centered.
                lv_obj_align(g_time_label, LV_ALIGN_TOP_MID, 0, show_ended ? 44 : (controls ? 42 : 70));
            }
            g_last_view = view;
        }
    } // namespace

    void timer_page_init()
    {
        build_screen();
    }

    void timer_page_show()
    {
        lv_scr_load(g_screen);
        sync(true);
    }

    void timer_page_hide()
    {
        // Retain the screen and controls for the next visit.
    }

    void timer_page_tick(uint32_t now_ms)
    {
        (void)now_ms;
        sync(false);
    }

} // namespace blob_native
