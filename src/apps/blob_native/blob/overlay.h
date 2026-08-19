#ifndef BLOB_NATIVE_OVERLAY_H
#define BLOB_NATIVE_OVERLAY_H

#include <stdint.h>

namespace blob_native
{

    void overlay_expand_dirty_bounds(int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);
    void overlay_draw(uint16_t *fb, uint16_t fps, int dirty_w, int dirty_h);

} // namespace blob_native

#endif
