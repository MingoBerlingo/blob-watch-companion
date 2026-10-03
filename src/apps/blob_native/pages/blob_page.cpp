#include "apps/blob_native/pages/blob_page.h"

#include <Arduino.h>

#include "apps/blob_native/blob/view.h"

namespace blob_native
{

    void blob_page_reset()
    {
        blob_screen_reset();
    }

    void blob_page_loop(uint32_t frame_start_us)
    {
        BlobScreenFrame frame;
        if (!blob_screen_prepare_frame(&frame))
        {
            delay(16);
            return;
        }

        blob_screen_render_frame(frame);
        blob_screen_commit_frame(frame);

        const uint32_t elapsed_us = micros() - frame_start_us;
        constexpr uint32_t FRAME_TARGET_US = 16667u;
        if (elapsed_us < FRAME_TARGET_US)
        {
            delayMicroseconds(FRAME_TARGET_US - elapsed_us);
        }
    }

} // namespace blob_native
