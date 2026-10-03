#ifndef BLOB_NATIVE_FACE_H
#define BLOB_NATIVE_FACE_H

#include <stdint.h>

namespace blob_native
{

    void compute_eye_positions(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                               int16_t *left_x, int16_t *left_y, int16_t *right_x, int16_t *right_y);
    void compute_mouth_points(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                              int16_t *x0, int16_t *y0, int16_t *xm, int16_t *ym, int16_t *x1, int16_t *y1);
    float compute_blink_amount(float motion_speed, float face_phase);

} // namespace blob_native

#endif
