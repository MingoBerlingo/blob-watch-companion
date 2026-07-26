#ifndef BLOB_NATIVE_FRAMEBUFFER_H
#define BLOB_NATIVE_FRAMEBUFFER_H

#include "apps/blob_native/blob_native_state.h"

namespace blob_native
{

    int rect_width(const Rect &rect);
    int rect_height(const Rect &rect);
    Rect rect_union(const Rect &a, const Rect &b);

    Rect make_blob_rect(float cx, float cy, float eye_x, float eye_y, float motion_speed, float face_phase,
                        const int16_t *px, const int16_t *py);

    void save_region_pixels(const Rect &rect);
    void restore_region_pixels(const Rect &rect);

} // namespace blob_native

#endif