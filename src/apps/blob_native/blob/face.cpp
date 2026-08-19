#include "apps/blob_native/blob/face.h"

#include <Arduino.h>
#include <math.h>

#include "apps/blob_native/shared_state.h"
#include "apps/blob_native/ui/draw.h"

namespace blob_native
{

    void normalize_vec(float *x, float *y)
    {
        const float len = sqrtf((*x) * (*x) + (*y) * (*y));
        if (len < 0.0001f)
        {
            *x = FACE_DIR_X;
            *y = FACE_DIR_Y;
            return;
        }

        *x /= len;
        *y /= len;
    }

    void update_point_bounds(int16_t x, int16_t y, int16_t radius, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
    {
        if (x - radius < *min_x)
            *min_x = x - radius;
        if (y - radius < *min_y)
            *min_y = y - radius;
        if (x + radius > *max_x)
            *max_x = x + radius;
        if (y + radius > *max_y)
            *max_y = y + radius;
    }

    float compute_idle_amount(float motion_speed)
    {
        return 1.0f - constrain(motion_speed / FACE_IDLE_SPEED_MAX, 0.0f, 1.0f);
    }

    float compute_blink_amount(float motion_speed, float face_phase)
    {
        const float idle_amount = compute_idle_amount(motion_speed);
        const float blink_wave = 0.5f + 0.5f * sinf(face_phase * FACE_IDLE_BLINK_RATE);
        if (blink_wave <= FACE_IDLE_BLINK_THRESHOLD)
        {
            return 0.0f;
        }

        const float blink = (blink_wave - FACE_IDLE_BLINK_THRESHOLD) / (1.0f - FACE_IDLE_BLINK_THRESHOLD);
        return constrain(blink * idle_amount, 0.0f, 1.0f);
    }

    void compute_eye_positions(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                               int16_t *left_x, int16_t *left_y, int16_t *right_x, int16_t *right_y)
    {
        normalize_vec(&dir_x, &dir_y);

        const float side_x = -dir_y;
        const float side_y = dir_x;
        const float idle_amount = compute_idle_amount(motion_speed);
        const float idle_forward = cosf(face_phase * 1.3f) * FACE_IDLE_EYE_BOB * idle_amount;
        const float eye_forward = EYE_FORWARD + idle_forward;

        *left_x = (int16_t)(cx + dir_x * eye_forward - side_x * EYE_SIDE);
        *left_y = (int16_t)(cy + dir_y * eye_forward - side_y * EYE_SIDE);
        *right_x = (int16_t)(cx + dir_x * eye_forward + side_x * EYE_SIDE);
        *right_y = (int16_t)(cy + dir_y * eye_forward + side_y * EYE_SIDE);
    }

    void draw_cross_eye(int16_t cx, int16_t cy, float side_x, float side_y, float dir_x, float dir_y, uint16_t color)
    {
        const int16_t half = EYE_RADIUS + 1;
        const int16_t x0 = (int16_t)(cx - side_x * half - dir_x * half);
        const int16_t y0 = (int16_t)(cy - side_y * half - dir_y * half);
        const int16_t x1 = (int16_t)(cx + side_x * half + dir_x * half);
        const int16_t y1 = (int16_t)(cy + side_y * half + dir_y * half);
        const int16_t x2 = (int16_t)(cx - side_x * half + dir_x * half);
        const int16_t y2 = (int16_t)(cy - side_y * half + dir_y * half);
        const int16_t x3 = (int16_t)(cx + side_x * half - dir_x * half);
        const int16_t y3 = (int16_t)(cy + side_y * half - dir_y * half);

        draw_line(x0, y0, x1, y1, color);
        draw_line(x2, y2, x3, y3, color);
    }

    void draw_blob_eyes(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                        EyesAnimState eyes_state, uint16_t color)
    {
        int16_t left_x, left_y, right_x, right_y;
        compute_eye_positions(cx, cy, dir_x, dir_y, motion_speed, face_phase, &left_x, &left_y, &right_x, &right_y);

        normalize_vec(&dir_x, &dir_y);
        float side_x = -dir_y;
        float side_y = dir_x;
        normalize_vec(&side_x, &side_y);

        if (eyes_state == EyesAnimState::ShakeX)
        {
            draw_cross_eye(left_x, left_y, side_x, side_y, dir_x, dir_y, color);
            draw_cross_eye(right_x, right_y, side_x, side_y, dir_x, dir_y, color);
            return;
        }

        if (eyes_state == EyesAnimState::BlinkClosed)
        {
            const int16_t half_lid = EYE_RADIUS + 1;

            const int16_t lx0 = (int16_t)(left_x - side_x * half_lid);
            const int16_t ly0 = (int16_t)(left_y - side_y * half_lid);
            const int16_t lx1 = (int16_t)(left_x + side_x * half_lid);
            const int16_t ly1 = (int16_t)(left_y + side_y * half_lid);

            const int16_t rx0 = (int16_t)(right_x - side_x * half_lid);
            const int16_t ry0 = (int16_t)(right_y - side_y * half_lid);
            const int16_t rx1 = (int16_t)(right_x + side_x * half_lid);
            const int16_t ry1 = (int16_t)(right_y + side_y * half_lid);

            draw_line(lx0, ly0, lx1, ly1, color);
            draw_line(rx0, ry0, rx1, ry1, color);
            return;
        }

        const float blink_amount = compute_blink_amount(motion_speed, face_phase);
        if (blink_amount > 0.4f)
        {
            const int16_t half_lid = EYE_RADIUS + 1;

            const int16_t lx0 = (int16_t)(left_x - side_x * half_lid);
            const int16_t ly0 = (int16_t)(left_y - side_y * half_lid);
            const int16_t lx1 = (int16_t)(left_x + side_x * half_lid);
            const int16_t ly1 = (int16_t)(left_y + side_y * half_lid);

            const int16_t rx0 = (int16_t)(right_x - side_x * half_lid);
            const int16_t ry0 = (int16_t)(right_y - side_y * half_lid);
            const int16_t rx1 = (int16_t)(right_x + side_x * half_lid);
            const int16_t ry1 = (int16_t)(right_y + side_y * half_lid);

            draw_line(lx0, ly0, lx1, ly1, color);
            draw_line(rx0, ry0, rx1, ry1, color);
            return;
        }

        draw_circle(left_x, left_y, EYE_RADIUS, color, true);
        draw_circle(right_x, right_y, EYE_RADIUS, color, true);
    }

    void update_eyes_bounds(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                            EyesAnimState eyes_state,
                            int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
    {
        int16_t left_x, left_y, right_x, right_y;
        compute_eye_positions(cx, cy, dir_x, dir_y, motion_speed, face_phase, &left_x, &left_y, &right_x, &right_y);

        int16_t extra = EYE_RADIUS + 1;
        if (eyes_state == EyesAnimState::ShakeX)
        {
            extra = EYE_RADIUS + 2;
        }

        update_point_bounds(left_x, left_y, extra, min_x, min_y, max_x, max_y);
        update_point_bounds(right_x, right_y, extra, min_x, min_y, max_x, max_y);
    }

    void compute_mouth_points(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                              int16_t *x0, int16_t *y0, int16_t *xm, int16_t *ym, int16_t *x1, int16_t *y1)
    {
        normalize_vec(&dir_x, &dir_y);
        const float side_x = -dir_y;
        const float side_y = dir_x;
        const float idle_amount = compute_idle_amount(motion_speed);
        const float idle_bob = sinf(face_phase * 1.1f) * FACE_IDLE_MOUTH_BOB * idle_amount;

        const float mouth_cx = cx + dir_x * (MOUTH_FORWARD + idle_bob);
        const float mouth_cy = cy + dir_y * (MOUTH_FORWARD + idle_bob);

        *x0 = (int16_t)(mouth_cx - side_x * MOUTH_HALF_LEN);
        *y0 = (int16_t)(mouth_cy - side_y * MOUTH_HALF_LEN);
        *xm = (int16_t)(mouth_cx - dir_x * MOUTH_SMILE_DEPTH);
        *ym = (int16_t)(mouth_cy - dir_y * MOUTH_SMILE_DEPTH);
        *x1 = (int16_t)(mouth_cx + side_x * MOUTH_HALF_LEN);
        *y1 = (int16_t)(mouth_cy + side_y * MOUTH_HALF_LEN);
    }

    void draw_blob_mouth(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                         MouthAnimState mouth_state, uint16_t color)
    {
        normalize_vec(&dir_x, &dir_y);

        if (mouth_state == MouthAnimState::OpenO)
        {
            const float mouth_cx = cx + dir_x * MOUTH_FORWARD;
            const float mouth_cy = cy + dir_y * MOUTH_FORWARD;
            const int16_t radius = (int16_t)(MOUTH_HALF_LEN * 0.55f);
            draw_circle((int)mouth_cx, (int)mouth_cy, radius, color, false);
            return;
        }

        int16_t x0, y0, xm, ym, x1, y1;
        compute_mouth_points(cx, cy, dir_x, dir_y, motion_speed, face_phase, &x0, &y0, &xm, &ym, &x1, &y1);
        draw_line(x0, y0, xm, ym, color);
        draw_line(xm, ym, x1, y1, color);
    }

    void update_mouth_bounds(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                             MouthAnimState mouth_state,
                             int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
    {
        normalize_vec(&dir_x, &dir_y);

        if (mouth_state == MouthAnimState::OpenO)
        {
            const int16_t mx = (int16_t)(cx + dir_x * MOUTH_FORWARD);
            const int16_t my = (int16_t)(cy + dir_y * MOUTH_FORWARD);
            const int16_t radius = (int16_t)(MOUTH_HALF_LEN * 0.55f) + 1;
            update_point_bounds(mx, my, radius, min_x, min_y, max_x, max_y);
            return;
        }

        int16_t x0, y0, xm, ym, x1, y1;
        compute_mouth_points(cx, cy, dir_x, dir_y, motion_speed, face_phase, &x0, &y0, &xm, &ym, &x1, &y1);
        update_point_bounds(x0, y0, 1, min_x, min_y, max_x, max_y);
        update_point_bounds(xm, ym, 1, min_x, min_y, max_x, max_y);
        update_point_bounds(x1, y1, 1, min_x, min_y, max_x, max_y);
    }

} // namespace blob_native