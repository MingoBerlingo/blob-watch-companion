#include "apps/blob_native/pages/blob/blob_page.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>

#include "lvgl.h"

#include "apps/blob_native/blob/face.h"
#include "apps/blob_native/blob/shape.h"
#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/shared_state.h"
#include "platform/waveshare_native_board.h"

namespace blob_native
{

    namespace
    {
        constexpr uint32_t BLOB_TICK_MS = 16u;
        constexpr float BLOB_PHASE_STEP = 0.08f;
        constexpr int16_t BLOB_BOUNDS_MARGIN = 2;

        struct BlobState
        {
            float t;
            float cx;
            float cy;
            float speed;
            float smooth_x;
            float smooth_y;
            float vel_x;
            float vel_y;
        };

        BlobState g_state = {0.0f, CENTER_X, CENTER_Y, 0.0f, CENTER_X, CENTER_Y, 0.0f, 0.0f};

        int16_t g_px[POINTS];
        int16_t g_py[POINTS];

        lv_obj_t *g_screen = nullptr;
        lv_obj_t *g_blob_obj = nullptr;
        lv_timer_t *g_timer = nullptr;
        lv_obj_t *g_fps_label = nullptr;
        uint32_t g_fps_frames = 0;
        uint32_t g_fps_window_ms = 0;

        void update_blob_bounds();

        lv_color_t col(uint16_t c)
        {
            return lv_rgb565(c);
        }

        void draw_segment(lv_draw_ctx_t *ctx, lv_draw_line_dsc_t *dsc,
                          int16_t x0, int16_t y0, int16_t x1, int16_t y1, lv_color_t color)
        {
            dsc->color = color;
            lv_point_t p1 = {x0, y0};
            lv_point_t p2 = {x1, y1};
            lv_draw_line(ctx, dsc, &p1, &p2);
        }

        void draw_blob(lv_event_t *e)
        {
            lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
            if (ctx == nullptr)
            {
                return;
            }

            const float cx = g_state.cx;
            const float cy = g_state.cy;
            const float speed = g_state.speed;
            const float phase = g_state.t;

            lv_draw_line_dsc_t line_dsc;
            lv_draw_line_dsc_init(&line_dsc);
            line_dsc.width = 1;

            // Glow: scaled contour outlines (optional; off for a crisper blob).
            if (BLOB_GLOW_ENABLED)
            {
                for (int layer = 0; layer < GLOW_LAYER_COUNT; layer++)
                {
                    int16_t glow_px[POINTS];
                    int16_t glow_py[POINTS];
                    scale_blob_points(cx, cy, GLOW_LAYER_SCALE[layer], g_px, g_py, glow_px, glow_py);
                    const lv_color_t c = col(GLOW_LAYER_COLOR[layer]);
                    for (int i = 0; i < POINTS; i++)
                    {
                        const int next = (i + 1) % POINTS;
                        draw_segment(ctx, &line_dsc, glow_px[i], glow_py[i], glow_px[next], glow_py[next], c);
                    }
                }
            }

            // Outline.
            const lv_color_t outline = lv_theme_get_color_primary(g_screen);
            for (int i = 0; i < POINTS; i++)
            {
                const int next = (i + 1) % POINTS;
                draw_segment(ctx, &line_dsc, g_px[i], g_py[i], g_px[next], g_py[next], outline);
            }

            // Eyes.
            int16_t lx, ly, rx, ry;
            compute_eye_positions(cx, cy, FACE_DIR_X, FACE_DIR_Y, speed, phase, &lx, &ly, &rx, &ry);

            const lv_color_t eye_c = lv_theme_get_color_secondary(g_screen);
            if (compute_blink_amount(speed, phase) > 0.4f)
            {
                const int16_t half_lid = EYE_RADIUS + 1;
                draw_segment(ctx, &line_dsc, (int16_t)(lx - half_lid), ly, (int16_t)(lx + half_lid), ly, eye_c);
                draw_segment(ctx, &line_dsc, (int16_t)(rx - half_lid), ry, (int16_t)(rx + half_lid), ry, eye_c);
            }
            else
            {
                lv_draw_rect_dsc_t rect_dsc;
                lv_draw_rect_dsc_init(&rect_dsc);
                rect_dsc.bg_opa = LV_OPA_COVER;
                rect_dsc.bg_color = eye_c;
                rect_dsc.radius = LV_RADIUS_CIRCLE;

                lv_area_t a1 = {(lv_coord_t)(lx - EYE_RADIUS), (lv_coord_t)(ly - EYE_RADIUS),
                                (lv_coord_t)(lx + EYE_RADIUS), (lv_coord_t)(ly + EYE_RADIUS)};
                lv_area_t a2 = {(lv_coord_t)(rx - EYE_RADIUS), (lv_coord_t)(ry - EYE_RADIUS),
                                (lv_coord_t)(rx + EYE_RADIUS), (lv_coord_t)(ry + EYE_RADIUS)};
                lv_draw_rect(ctx, &rect_dsc, &a1);
                lv_draw_rect(ctx, &rect_dsc, &a2);
            }

            // Mouth (neutral smile).
            int16_t mx0, my0, mxm, mym, mx1, my1;
            compute_mouth_points(cx, cy, FACE_DIR_X, FACE_DIR_Y, speed, phase, &mx0, &my0, &mxm, &mym, &mx1, &my1);
            const lv_color_t mouth_c = lv_theme_get_color_secondary(g_screen);
            draw_segment(ctx, &line_dsc, mx0, my0, mxm, mym, mouth_c);
            draw_segment(ctx, &line_dsc, mxm, mym, mx1, my1, mouth_c);
        }

        void advance_blob()
        {
            float cx = CENTER_X + cosf(g_state.t * 0.7f) * 24.0f;
            float cy = CENTER_Y + sinf(g_state.t * 0.9f) * 18.0f;
            float vx = cosf(g_state.t * 1.2f) * 0.8f;
            float vy = sinf(g_state.t * 1.1f) * 0.8f;

            if (BLOB_USE_IMU)
            {
                float tilt_x = 0.0f;
                float tilt_y = 0.0f;
                if (waveshare_native_read_tilt(&tilt_x, &tilt_y))
                {
                    const float target_x = CENTER_X + tilt_y * (CENTER_X - BLOB_RADIUS - 6.0f);
                    const float target_y = CENTER_Y - tilt_x * (CENTER_Y - BLOB_RADIUS - 6.0f);

                    g_state.vel_x = g_state.vel_x * 0.75f + (target_x - g_state.smooth_x) * 0.15f;
                    g_state.vel_y = g_state.vel_y * 0.75f + (target_y - g_state.smooth_y) * 0.15f;
                    g_state.smooth_x += g_state.vel_x;
                    g_state.smooth_y += g_state.vel_y;

                    cx = g_state.smooth_x;
                    cy = g_state.smooth_y;
                    vx = g_state.vel_x;
                    vy = g_state.vel_y;
                }
            }

            g_state.cx = cx;
            g_state.cy = cy;
            g_state.speed = sqrtf(vx * vx + vy * vy);

            compute_blob(cx, cy, vx, vy, g_state.t, g_px, g_py);
            g_state.t += BLOB_PHASE_STEP;

            // Track the blob's exact bounding box so only the changed region is redrawn.
            update_blob_bounds();
        }

        void reset_blob()
        {
            g_state.t = 0.0f;
            g_state.smooth_x = CENTER_X;
            g_state.smooth_y = CENTER_Y;
            g_state.vel_x = 0.0f;
            g_state.vel_y = 0.0f;
            g_state.cx = CENTER_X;
            g_state.cy = CENTER_Y;
            g_state.speed = 0.0f;
            compute_blob(CENTER_X, CENTER_Y, 0.0f, 0.0f, 0.0f, g_px, g_py);
        }

        void compute_point_bounds(const int16_t *px, const int16_t *py, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
        {
            *min_x = SCREEN_W - 1;
            *min_y = SCREEN_H - 1;
            *max_x = 0;
            *max_y = 0;
            for (int i = 0; i < POINTS; i++)
            {
                if (px[i] < *min_x)
                    *min_x = px[i];
                if (py[i] < *min_y)
                    *min_y = py[i];
                if (px[i] > *max_x)
                    *max_x = px[i];
                if (py[i] > *max_y)
                    *max_y = py[i];
            }
        }

        void update_blob_bounds()
        {
            int16_t min_x, min_y, max_x, max_y;

            if (BLOB_GLOW_ENABLED)
            {
                int16_t glow_px[POINTS];
                int16_t glow_py[POINTS];
                scale_blob_points(g_state.cx, g_state.cy, GLOW_LAYER_SCALE[0], g_px, g_py, glow_px, glow_py);
                compute_point_bounds(glow_px, glow_py, &min_x, &min_y, &max_x, &max_y);
            }
            else
            {
                compute_point_bounds(g_px, g_py, &min_x, &min_y, &max_x, &max_y);
            }

            min_x -= BLOB_BOUNDS_MARGIN;
            min_y -= BLOB_BOUNDS_MARGIN;
            max_x += BLOB_BOUNDS_MARGIN;
            max_y += BLOB_BOUNDS_MARGIN;

            lv_obj_set_pos(g_blob_obj, min_x, min_y);
            lv_obj_set_size(g_blob_obj, (lv_coord_t)(max_x - min_x + 1), (lv_coord_t)(max_y - min_y + 1));
        }

        void blob_timer_cb(lv_timer_t *timer)
        {
            (void)timer;
            advance_blob();
            lv_obj_invalidate(g_blob_obj);

            if (BLOB_PERF_OVERLAY_ENABLED && g_fps_label != nullptr)
            {
                g_fps_frames++;
                const uint32_t now_ms = millis();
                if (g_fps_window_ms == 0)
                {
                    g_fps_window_ms = now_ms;
                }
                const uint32_t elapsed = now_ms - g_fps_window_ms;
                if (elapsed >= BLOB_PERF_OVERLAY_UPDATE_MS)
                {
                    const unsigned int fps = (unsigned int)((g_fps_frames * 1000u) / elapsed);
                    lv_label_set_text_fmt(g_fps_label, "FPS %u", fps);
                    g_fps_frames = 0;
                    g_fps_window_ms = now_ms;
                }
            }
        }
    } // namespace

    void blob_page_init()
    {
        g_screen = lv_obj_create(nullptr);
        lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC |
                                    LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_WITH_ARROW |
                                    LV_OBJ_FLAG_SNAPPABLE);
        lv_obj_set_style_bg_color(g_screen, col(BG_COLOR), 0);
        lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);

        g_blob_obj = lv_obj_create(g_screen);
        lv_obj_clear_flag(g_blob_obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(g_blob_obj, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(g_blob_obj, 0, 0);
        lv_obj_add_event_cb(g_blob_obj, draw_blob, LV_EVENT_DRAW_MAIN, nullptr);

        if (BLOB_PERF_OVERLAY_ENABLED)
        {
            g_fps_label = lv_label_create(g_screen);
            lv_obj_align(g_fps_label, LV_ALIGN_TOP_LEFT, BLOB_PERF_OVERLAY_X, BLOB_PERF_OVERLAY_Y);
            lv_obj_set_style_text_font(g_fps_label, &lv_font_montserrat_16, 0);
            lv_label_set_text(g_fps_label, "");
            lv_obj_move_foreground(g_fps_label);
        }

        reset_blob();
        update_blob_bounds();

        g_timer = lv_timer_create(blob_timer_cb, BLOB_TICK_MS, nullptr);
        lv_timer_pause(g_timer);
    }

    void blob_page_show()
    {
        reset_blob();
        update_blob_bounds();
        lv_scr_load(g_screen);
        lv_timer_resume(g_timer);
    }

    void blob_page_hide()
    {
        lv_timer_pause(g_timer);
    }

} // namespace blob_native
