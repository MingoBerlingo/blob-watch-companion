#include "apps/blob_native/blob/face.h"

#include <Arduino.h>
#include <math.h>

#include "apps/blob_native/shared_state.h"

namespace blob_native
{

    static void normalize_vec(float *x, float *y)
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

    static float compute_idle_amount(float motion_speed)
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

} // namespace blob_native
