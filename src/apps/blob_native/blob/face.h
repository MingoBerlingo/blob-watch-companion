#ifndef BLOB_NATIVE_FACE_H
#define BLOB_NATIVE_FACE_H

#include <stdint.h>

#include "apps/blob_native/shared_state.h"

namespace blob_native
{

    void draw_blob_eyes(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                        EyesAnimState eyes_state, uint16_t color);
    void update_eyes_bounds(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                            EyesAnimState eyes_state,
                            int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);

    void draw_blob_mouth(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                         MouthAnimState mouth_state, uint16_t color);
    void update_mouth_bounds(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                             MouthAnimState mouth_state,
                             int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);

} // namespace blob_native

#endif