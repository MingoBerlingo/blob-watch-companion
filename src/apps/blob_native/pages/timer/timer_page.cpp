#include "apps/blob_native/pages/timer/timer_page.h"

#include <stdio.h>

#include "lvgl.h"

#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/shared_state.h"
#include "apps/blob_native/timer/view.h"

namespace blob_native
{

    namespace
    {
        constexpr int kRollerOptions = 100; // "00" .. "99"

        lv_obj_t *g_screen = nullptr;

        lv_obj_t *g_minutes_roller = nullptr;
        lv_obj_t *g_seconds_roller = nullptr;
        lv_obj_t *g_start_btn = nullptr;

        lv_obj_t *g_arc = nullptr;
        lv_obj_t *g_time_label = nullptr;
        lv_obj_t *g_msgbox = nullptr;

        // Roller options string ("00\n01\n...\n99") and msgbox button labels.
        char g_roller_options[4 * kRollerOptions];
        const char *g_msgbox_btn_map[4] = {"Back", "Play", "Cancel", ""};

        // Change tracking.
        int g_last_minutes = -1;
        int g_last_seconds = -1;
        int32_t g_last_display_seconds = -1;
        int g_last_arc_value = -1;
        bool g_last_running = false;

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

        void on_screen_clicked(lv_event_t *e)
        {
            (void)e;
            if (timer_view() == TimerView::TimerRun)
            {
                timer_toggle_controls();
            }
        }

        void on_msgbox_value_changed(lv_event_t *e)
        {
            lv_obj_t *btnm = lv_event_get_target(e);
            const uint16_t btn_id = lv_btnmatrix_get_selected_btn(btnm);

            if (btn_id == 0)
            {
                timer_hide_controls();
            }
            else if (btn_id == 1)
            {
                timer_action(millis());
            }
            else if (btn_id == 2)
            {
                timer_cancel();
            }
        }

        void update_msgbox_buttons(bool running)
        {
            g_msgbox_btn_map[1] = running ? "Pause" : "Play";
            lv_btnmatrix_set_map(lv_msgbox_get_btns(g_msgbox), g_msgbox_btn_map);
        }

        void build_screen()
        {
            build_roller_options();

            g_screen = lv_obj_create(nullptr);
            lv_obj_add_flag(g_screen, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(g_screen, col(BG_COLOR), 0);
            lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);

            // --- Setup view ---
            g_minutes_roller = lv_roller_create(g_screen);
            lv_roller_set_options(g_minutes_roller, g_roller_options, LV_ROLLER_MODE_NORMAL);
            lv_roller_set_visible_row_count(g_minutes_roller, 3);
            lv_obj_set_pos(g_minutes_roller, 40, 50);
            lv_obj_set_size(g_minutes_roller, 70, 120);
            lv_obj_add_event_cb(g_minutes_roller, on_minutes_changed, LV_EVENT_VALUE_CHANGED, nullptr);

            g_seconds_roller = lv_roller_create(g_screen);
            lv_roller_set_options(g_seconds_roller, g_roller_options, LV_ROLLER_MODE_NORMAL);
            lv_roller_set_visible_row_count(g_seconds_roller, 3);
            lv_obj_set_pos(g_seconds_roller, 130, 50);
            lv_obj_set_size(g_seconds_roller, 70, 120);
            lv_obj_add_event_cb(g_seconds_roller, on_seconds_changed, LV_EVENT_VALUE_CHANGED, nullptr);

            g_start_btn = lv_btn_create(g_screen);
            lv_obj_set_pos(g_start_btn, 70, 190);
            lv_obj_set_size(g_start_btn, 100, 44);
            lv_obj_add_event_cb(g_start_btn, on_start_clicked, LV_EVENT_CLICKED, nullptr);

            lv_obj_t *start_label = lv_label_create(g_start_btn);
            lv_label_set_text(start_label, "Start");
            lv_obj_center(start_label);

            // --- Run view ---
            g_arc = lv_arc_create(g_screen);
            lv_obj_clear_flag(g_arc, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_pos(g_arc, 2, 2);
            lv_obj_set_size(g_arc, 236, 236);
            lv_obj_set_style_pad_all(g_arc, 0, 0);
            lv_arc_set_bg_angles(g_arc, 0, 360);
            lv_arc_set_rotation(g_arc, 270);
            lv_arc_set_range(g_arc, 0, 10000);
            lv_arc_set_value(g_arc, 0);
            lv_arc_set_mode(g_arc, LV_ARC_MODE_NORMAL);
            lv_obj_set_style_arc_width(g_arc, 8, LV_PART_MAIN);
            lv_obj_set_style_arc_width(g_arc, 8, LV_PART_INDICATOR);
            lv_obj_set_style_arc_color(g_arc, col(BG_COLOR), LV_PART_MAIN);
            lv_obj_set_style_arc_color(g_arc, lv_theme_get_color_primary(g_screen), LV_PART_INDICATOR);
            lv_obj_set_style_bg_opa(g_arc, LV_OPA_TRANSP, 0);
            lv_obj_remove_style(g_arc, nullptr, LV_PART_KNOB);

            g_time_label = lv_label_create(g_screen);
            lv_obj_clear_flag(g_time_label, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_text_font(g_time_label, &lv_font_montserrat_24, 0);
            lv_obj_align(g_time_label, LV_ALIGN_CENTER, 0, 0);

            // Controls message box (Back / Pause-Play / Cancel).
            g_msgbox = lv_msgbox_create(g_screen, "Timer", "", g_msgbox_btn_map, false);
            lv_obj_set_width(g_msgbox, 200);
            lv_obj_center(g_msgbox);
            lv_obj_add_event_cb(lv_msgbox_get_btns(g_msgbox), on_msgbox_value_changed, LV_EVENT_VALUE_CHANGED, nullptr);

            lv_obj_add_event_cb(g_screen, on_screen_clicked, LV_EVENT_CLICKED, nullptr);
        }

        void sync(bool force)
        {
            const TimerView view = timer_view();
            const bool controls = timer_run_controls_visible();
            const bool show_setup = (view == TimerView::TimerSetup);
            const bool show_run = (view == TimerView::TimerRun);

            set_visible(g_minutes_roller, show_setup);
            set_visible(g_seconds_roller, show_setup);
            set_visible(g_start_btn, show_setup);
            set_visible(g_arc, show_run);
            set_visible(g_time_label, show_run);
            set_visible(g_msgbox, show_run && controls);

            if (show_setup)
            {
                const int mins = timer_minutes();
                const int secs = timer_seconds();
                if (force || mins != g_last_minutes)
                {
                    lv_roller_set_selected(g_minutes_roller, (uint16_t)mins, LV_ANIM_OFF);
                    g_last_minutes = mins;
                }
                if (force || secs != g_last_seconds)
                {
                    lv_roller_set_selected(g_seconds_roller, (uint16_t)secs, LV_ANIM_OFF);
                    g_last_seconds = secs;
                }
            }

            if (show_run)
            {
                const int32_t disp = timer_display_seconds_value();
                if (force || disp != g_last_display_seconds)
                {
                    char buf[12];
                    snprintf(buf, sizeof(buf), "%02ld:%02ld", (long)(disp / 60), (long)(disp % 60));
                    lv_label_set_text(g_time_label, buf);
                    g_last_display_seconds = disp;
                }

                const uint32_t total = timer_total_ms();
                const uint32_t remaining = timer_remaining_ms();
                int value = 0;
                if (total > 0)
                {
                    const int64_t scaled = ((int64_t)remaining * 10000) / (int64_t)total;
                    value = (int)scaled;
                    if (value < 0)
                        value = 0;
                    if (value > 10000)
                        value = 10000;
                }
                if (force || value != g_last_arc_value)
                {
                    lv_arc_set_value(g_arc, (int16_t)value);
                    g_last_arc_value = value;
                }

                const bool running = timer_running();
                if (force || running != g_last_running)
                {
                    update_msgbox_buttons(running);
                    g_last_running = running;
                }
            }
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
        // Nothing to do; the page manager stops driving LVGL while the blob page owns the framebuffer.
    }

    void timer_page_tick(uint32_t now_ms)
    {
        (void)now_ms;
        sync(false);
    }

} // namespace blob_native
