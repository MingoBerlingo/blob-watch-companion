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
        constexpr int kRollerOptions = 60; // "00" .. "59"

        lv_obj_t *g_screen = nullptr;

        lv_obj_t *g_minutes_roller = nullptr;
        lv_obj_t *g_seconds_roller = nullptr;
        lv_obj_t *g_start_btn = nullptr;

        lv_obj_t *g_arc = nullptr;
        lv_obj_t *g_time_label = nullptr;
        lv_obj_t *g_msgbox = nullptr;
        lv_obj_t *g_back_btn = nullptr;
        lv_obj_t *g_play_btn = nullptr;
        lv_obj_t *g_cancel_btn = nullptr;
        lv_obj_t *g_play_label = nullptr;

        // Roller options string ("00\n01\n...\n99") and msgbox button labels.
        char g_roller_options[4 * kRollerOptions];

        // Change tracking.
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

        void on_back_clicked(lv_event_t *e)
        {
            (void)e;
            timer_hide_controls();
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

        void update_play_button(bool running)
        {
            lv_label_set_text(g_play_label, running ? "Pause" : "Play");
        }

        lv_obj_t *add_control_button(lv_obj_t *parent, const char *text, lv_color_t color)
        {
            lv_obj_t *btn = lv_btn_create(parent);
            lv_obj_set_size(btn, 68, 48);
            lv_obj_set_style_pad_hor(btn, 4, 0);
            lv_obj_set_style_bg_color(btn, color, 0);
            lv_obj_set_style_bg_color(btn, lv_color_darken(color, LV_OPA_30), LV_STATE_PRESSED);

            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text(label, text);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(label, lv_color_white(), 0);
            lv_obj_center(label);

            return btn;
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

            // Controls message box (Back / Pause-Play / Cancel) in a flex column.
            g_msgbox = lv_msgbox_create(g_screen, "", "", nullptr, false);
            lv_obj_set_width(g_msgbox, 120);
            lv_obj_center(g_msgbox);

            lv_obj_t *controls_content = lv_msgbox_get_content(g_msgbox);
            lv_obj_set_flex_flow(controls_content, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(controls_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_row(controls_content, 8, 0);

            g_back_btn = add_control_button(controls_content, "Back", lv_color_hex(0x616161));
            g_play_btn = add_control_button(controls_content, "Play", lv_color_hex(0xFF9800));
            g_cancel_btn = add_control_button(controls_content, "Cancel", lv_color_hex(0xF44336));
            g_play_label = lv_obj_get_child(g_play_btn, 0);

            lv_obj_add_event_cb(g_back_btn, on_back_clicked, LV_EVENT_CLICKED, nullptr);
            lv_obj_add_event_cb(g_play_btn, on_play_clicked, LV_EVENT_CLICKED, nullptr);
            lv_obj_add_event_cb(g_cancel_btn, on_cancel_clicked, LV_EVENT_CLICKED, nullptr);

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

            if (show_setup && force)
            {
                lv_roller_set_selected(g_minutes_roller, (uint16_t)timer_minutes(), LV_ANIM_OFF);
                lv_roller_set_selected(g_seconds_roller, (uint16_t)timer_seconds(), LV_ANIM_OFF);
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
                    update_play_button(running);
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
