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
        // Native RGB565 colors (same encoding used across the project). They are
        // converted through lv_rgb565 so they render identically on the panel.
        constexpr uint16_t kColorBorderIdle = 0x39E7;
        constexpr uint16_t kColorBack = 0x31A6;
        constexpr uint16_t kColorActionRunning = 0xCE79; // pause fill
        constexpr uint16_t kColorActionPaused = 0x57EA;  // play fill
        constexpr uint16_t kColorCancel = 0xF98C;

        lv_obj_t *g_screen = nullptr;

        lv_obj_t *g_minutes_btn = nullptr;
        lv_obj_t *g_seconds_btn = nullptr;
        lv_obj_t *g_start_btn = nullptr;
        lv_obj_t *g_minutes_label = nullptr;
        lv_obj_t *g_seconds_label = nullptr;
        lv_obj_t *g_start_label = nullptr;

        lv_obj_t *g_arc = nullptr;
        lv_obj_t *g_time_label = nullptr;
        lv_obj_t *g_back_btn = nullptr;
        lv_obj_t *g_action_btn = nullptr;
        lv_obj_t *g_cancel_btn = nullptr;
        lv_obj_t *g_back_label = nullptr;
        lv_obj_t *g_action_label = nullptr;
        lv_obj_t *g_cancel_label = nullptr;

        bool g_drag_active = false;
        int16_t g_drag_start_y = 0;
        int g_drag_applied_steps = 0;

        // Change tracking to keep redraws minimal.
        int g_last_minutes = -1;
        int g_last_seconds = -1;
        int32_t g_last_display_seconds = -1;
        TimerField g_last_active_field = TimerField::Minutes;
        bool g_last_running = false;
        int g_last_arc_value = -1;

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

        void style_rect_btn(lv_obj_t *btn, uint16_t border_color)
        {
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_bg_color(btn, col(BG_COLOR), 0);
            lv_obj_set_style_border_width(btn, 1, 0);
            lv_obj_set_style_border_opa(btn, LV_OPA_COVER, 0);
            lv_obj_set_style_border_color(btn, col(border_color), 0);
            lv_obj_set_style_radius(btn, 0, 0);
            lv_obj_set_style_shadow_width(btn, 0, 0);
            lv_obj_set_style_outline_width(btn, 0, 0);
            lv_obj_set_style_pad_all(btn, 0, 0);
        }

        void style_filled_rect_btn(lv_obj_t *btn, uint16_t fill_color)
        {
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(btn, col(fill_color), 0);
            lv_obj_set_style_border_width(btn, 0, 0);
            lv_obj_set_style_radius(btn, 0, 0);
            lv_obj_set_style_shadow_width(btn, 0, 0);
            lv_obj_set_style_outline_width(btn, 0, 0);
            lv_obj_set_style_pad_all(btn, 0, 0);
        }

        void style_circle_btn(lv_obj_t *btn, uint16_t fill_color)
        {
            lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(btn, col(fill_color), 0);
            lv_obj_set_style_border_width(btn, 0, 0);
            lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_shadow_width(btn, 0, 0);
            lv_obj_set_style_outline_width(btn, 0, 0);
            lv_obj_set_style_pad_all(btn, 0, 0);
        }

        lv_point_t point_of(lv_event_t *e)
        {
            lv_point_t p = {0, 0};
            lv_indev_t *indev = lv_event_get_indev(e);
            if (indev != nullptr)
            {
                lv_indev_get_point(indev, &p);
            }
            return p;
        }

        void on_pressed(lv_event_t *e)
        {
            if (timer_view() != TimerView::TimerSetup)
            {
                g_drag_active = false;
                return;
            }

            lv_point_t p = point_of(e);
            g_drag_active = true;
            g_drag_start_y = p.y;
            g_drag_applied_steps = 0;
        }

        void on_pressing(lv_event_t *e)
        {
            if (!g_drag_active)
            {
                return;
            }

            lv_point_t p = point_of(e);
            const int dy = (int)g_drag_start_y - (int)p.y; // drag up = increase
            const int steps = dy / 10;
            const int delta = steps - g_drag_applied_steps;
            if (delta != 0)
            {
                timer_apply_value_delta(delta);
                g_drag_applied_steps = steps;
            }
        }

        void on_released(lv_event_t *e)
        {
            (void)e;
            g_drag_active = false;
        }

        void attach_drag(lv_obj_t *obj)
        {
            lv_obj_add_event_cb(obj, on_pressed, LV_EVENT_PRESSED, nullptr);
            lv_obj_add_event_cb(obj, on_pressing, LV_EVENT_PRESSING, nullptr);
            lv_obj_add_event_cb(obj, on_released, LV_EVENT_RELEASED, nullptr);
        }

        void on_minutes_clicked(lv_event_t *e)
        {
            (void)e;
            timer_select_minutes();
        }

        void on_seconds_clicked(lv_event_t *e)
        {
            (void)e;
            timer_select_seconds();
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

        void on_action_clicked(lv_event_t *e)
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
            g_screen = lv_obj_create(nullptr);
            lv_obj_add_flag(g_screen, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(g_screen, col(BG_COLOR), 0);
            lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);

            // --- Setup view ---
            g_minutes_btn = lv_btn_create(g_screen);
            lv_obj_set_pos(g_minutes_btn, 42, 58);
            lv_obj_set_size(g_minutes_btn, 70, 88);
            style_rect_btn(g_minutes_btn, kColorBorderIdle);
            lv_obj_add_event_cb(g_minutes_btn, on_minutes_clicked, LV_EVENT_CLICKED, nullptr);
            attach_drag(g_minutes_btn);

            g_minutes_label = lv_label_create(g_minutes_btn);
            lv_obj_set_style_text_color(g_minutes_label, col(WHITE), 0);
            lv_obj_set_style_text_font(g_minutes_label, &lv_font_montserrat_24, 0);
            lv_obj_center(g_minutes_label);

            g_seconds_btn = lv_btn_create(g_screen);
            lv_obj_set_pos(g_seconds_btn, 128, 58);
            lv_obj_set_size(g_seconds_btn, 70, 88);
            style_rect_btn(g_seconds_btn, kColorBorderIdle);
            lv_obj_add_event_cb(g_seconds_btn, on_seconds_clicked, LV_EVENT_CLICKED, nullptr);
            attach_drag(g_seconds_btn);

            g_seconds_label = lv_label_create(g_seconds_btn);
            lv_obj_set_style_text_color(g_seconds_label, col(WHITE), 0);
            lv_obj_set_style_text_font(g_seconds_label, &lv_font_montserrat_24, 0);
            lv_obj_center(g_seconds_label);

            g_start_btn = lv_btn_create(g_screen);
            lv_obj_set_pos(g_start_btn, 50, 170);
            lv_obj_set_size(g_start_btn, 140, 44);
            style_filled_rect_btn(g_start_btn, CYAN);
            lv_obj_add_event_cb(g_start_btn, on_start_clicked, LV_EVENT_CLICKED, nullptr);
            attach_drag(g_start_btn);

            g_start_label = lv_label_create(g_start_btn);
            lv_label_set_text(g_start_label, "START");
            lv_obj_set_style_text_color(g_start_label, col(BLACK), 0);
            lv_obj_set_style_text_font(g_start_label, &lv_font_montserrat_16, 0);
            lv_obj_center(g_start_label);

            // --- Run view ---
            g_arc = lv_arc_create(g_screen);
            lv_obj_clear_flag(g_arc, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_pos(g_arc, 2, 2);
            lv_obj_set_size(g_arc, 236, 236);
            lv_obj_set_style_pad_all(g_arc, 0, 0);
            lv_arc_set_bg_angles(g_arc, 0, 360);
            lv_arc_set_rotation(g_arc, 270);
            lv_arc_set_range(g_arc, 0, 1000);
            lv_arc_set_value(g_arc, 0);
            lv_arc_set_mode(g_arc, LV_ARC_MODE_NORMAL);
            lv_obj_set_style_arc_width(g_arc, 8, LV_PART_MAIN);
            lv_obj_set_style_arc_width(g_arc, 8, LV_PART_INDICATOR);
            lv_obj_set_style_arc_color(g_arc, col(BG_COLOR), LV_PART_MAIN);
            lv_obj_set_style_arc_color(g_arc, col(CYAN), LV_PART_INDICATOR);
            lv_obj_set_style_bg_opa(g_arc, LV_OPA_TRANSP, 0);
            lv_obj_remove_style(g_arc, nullptr, LV_PART_KNOB);

            g_time_label = lv_label_create(g_screen);
            lv_obj_clear_flag(g_time_label, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_text_color(g_time_label, col(WHITE), 0);
            lv_obj_set_style_text_font(g_time_label, &lv_font_montserrat_24, 0);
            lv_obj_align(g_time_label, LV_ALIGN_CENTER, 0, 6);

            g_back_btn = lv_btn_create(g_screen);
            lv_obj_set_pos(g_back_btn, 42, 44);
            lv_obj_set_size(g_back_btn, 68, 68);
            style_circle_btn(g_back_btn, kColorBack);
            lv_obj_add_event_cb(g_back_btn, on_back_clicked, LV_EVENT_CLICKED, nullptr);

            g_back_label = lv_label_create(g_back_btn);
            lv_label_set_text(g_back_label, LV_SYMBOL_LEFT);
            lv_obj_set_style_text_color(g_back_label, col(BLACK), 0);
            lv_obj_center(g_back_label);

            g_action_btn = lv_btn_create(g_screen);
            lv_obj_set_pos(g_action_btn, 130, 44);
            lv_obj_set_size(g_action_btn, 68, 68);
            style_circle_btn(g_action_btn, kColorActionPaused);
            lv_obj_add_event_cb(g_action_btn, on_action_clicked, LV_EVENT_CLICKED, nullptr);

            g_action_label = lv_label_create(g_action_btn);
            lv_label_set_text(g_action_label, LV_SYMBOL_PLAY);
            lv_obj_set_style_text_color(g_action_label, col(BLACK), 0);
            lv_obj_center(g_action_label);

            g_cancel_btn = lv_btn_create(g_screen);
            lv_obj_set_pos(g_cancel_btn, 86, 124);
            lv_obj_set_size(g_cancel_btn, 68, 68);
            style_circle_btn(g_cancel_btn, kColorCancel);
            lv_obj_add_event_cb(g_cancel_btn, on_cancel_clicked, LV_EVENT_CLICKED, nullptr);

            g_cancel_label = lv_label_create(g_cancel_btn);
            lv_label_set_text(g_cancel_label, LV_SYMBOL_CLOSE);
            lv_obj_set_style_text_color(g_cancel_label, col(BLACK), 0);
            lv_obj_center(g_cancel_label);

            lv_obj_add_event_cb(g_screen, on_screen_clicked, LV_EVENT_CLICKED, nullptr);
            attach_drag(g_screen);
        }

        void sync(bool force)
        {
            const TimerView view = timer_view();
            const bool controls = timer_run_controls_visible();
            const bool show_setup = (view == TimerView::TimerSetup);
            const bool show_run = (view == TimerView::TimerRun);
            const bool show_ctrl = show_run && controls;

            set_visible(g_minutes_btn, show_setup);
            set_visible(g_seconds_btn, show_setup);
            set_visible(g_start_btn, show_setup);
            set_visible(g_arc, show_run);
            set_visible(g_time_label, show_run);
            set_visible(g_back_btn, show_ctrl);
            set_visible(g_action_btn, show_ctrl);
            set_visible(g_cancel_btn, show_ctrl);

            if (show_setup)
            {
                const int mins = timer_minutes();
                const int secs = timer_seconds();
                if (force || mins != g_last_minutes)
                {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%02d", mins);
                    lv_label_set_text(g_minutes_label, buf);
                    g_last_minutes = mins;
                }
                if (force || secs != g_last_seconds)
                {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%02d", secs);
                    lv_label_set_text(g_seconds_label, buf);
                    g_last_seconds = secs;
                }

                const TimerField active = timer_active_field();
                if (force || active != g_last_active_field)
                {
                    style_rect_btn(g_minutes_btn, (active == TimerField::Minutes) ? (uint16_t)CYAN : kColorBorderIdle);
                    style_rect_btn(g_seconds_btn, (active == TimerField::Seconds) ? (uint16_t)CYAN : kColorBorderIdle);
                    g_last_active_field = active;
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
                    const int64_t scaled = ((int64_t)remaining * 1000) / (int64_t)total;
                    value = (int)scaled;
                    if (value < 0)
                        value = 0;
                    if (value > 1000)
                        value = 1000;
                }
                if (force || value != g_last_arc_value)
                {
                    lv_arc_set_value(g_arc, (int16_t)value);
                    g_last_arc_value = value;
                }

                const bool running = timer_running();
                if (force || running != g_last_running)
                {
                    style_circle_btn(g_action_btn, running ? kColorActionRunning : kColorActionPaused);
                    lv_label_set_text(g_action_label, running ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
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
        // Nothing to do: the page manager stops driving LVGL while the blob
        // page owns the framebuffer. The screen stays loaded and is re-synced
        // on the next show().
    }

    void timer_page_tick(uint32_t now_ms)
    {
        (void)now_ms;
        sync(false);
    }

} // namespace blob_native
