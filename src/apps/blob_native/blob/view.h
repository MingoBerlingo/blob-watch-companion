#ifndef BLOB_NATIVE_BLOB_SCREEN_H
#define BLOB_NATIVE_BLOB_SCREEN_H

#include <stdint.h>

#include "apps/blob_native/shared_state.h"

namespace blob_native
{

    struct BlobScreenFrame
    {
        float cx;
        float cy;
        float speed;
        float phase_t;
        int16_t px[POINTS];
        int16_t py[POINTS];
        int16_t min_x;
        int16_t min_y;
        int16_t max_x;
        int16_t max_y;
        bool overlay_redraw;
        uint16_t overlay_fps;
    };

    void blob_screen_reset();
    bool blob_screen_prepare_frame(BlobScreenFrame *frame);
    void blob_screen_render_frame(const BlobScreenFrame &frame);
    void blob_screen_commit_frame(const BlobScreenFrame &frame);

} // namespace blob_native

#endif
